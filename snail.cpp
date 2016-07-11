#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include "snail.h"

float derivative(float x) {
  float f = func(x);
  return f * (1.f - f);
}

float func(float x) {
  return 1.f / (1.f + exp(-x));
}

bool DataArray::Size::check(int x, int y, int z) const {
  return (x >= 0 && x < w) && (y >= 0 && y < h) && (z >= 0 && z < d);
}


bool DataArray::Size::operator==(const Size & size) const {
  return (w == size.w) && (h == size.h) && (d == size.d);
}

bool DataArray::Size::operator!=(const Size & size) const {
  return !((*this) == size);
}


DataArray::DataArray() {
  arr = 0;
}


DataArray::DataArray(Size size) : size(size) {
  arr = new float[size.elemCnt()];
}


DataArray::DataArray(int w, int h, int d) : size(w, h, d) {
  arr = new float[size.elemCnt()];
}


DataArray::DataArray(const DataArray & dataArray) {
  size = dataArray.size;
  arr = new float[size.elemCnt()];
  std::copy(dataArray.arr, dataArray.arr + dataArray.size.elemCnt(), arr);
}


DataArray::~DataArray() {
  if (arr)
    delete[] arr;
}


DataArray& DataArray::operator=(const DataArray & other) {
  if (other.arr == 0) {
    if (arr)
      delete[] arr;
    size = other.size;

    return *this;
  }

  if (size != other.size) {
    // Размеры не совпадают    
    
    if (arr)
      delete[] arr;

    arr = new float[other.size.elemCnt()];
    size = other.size;
  }

  std::copy(other.arr, other.arr + other.size.elemCnt(), arr);
  return *this;
}


float & DataArray::at(int x, int y, int z) {
  return arr[z * size.w * size.h + y * size.w + x];
}


const float & DataArray::at(int x, int y, int z) const {
  return arr[z * size.w * size.h + y * size.w + x];
}


void DataArray::clear() {
  std::fill(arr, arr + size.elemCnt(), 0);
}

void DataArray::fillRnd(float minVal, float maxVal) {
  std::generate_n(arr, size.elemCnt(), [minVal, maxVal] ()->float {
    return minVal + rand()/(RAND_MAX / (maxVal-minVal));
  });
}

void DataArray::addZeros(int width, DataArray & output) const {
  Size newSize = size;
  newSize.w += 2 * width;
  newSize.h += 2 * width;

  if (newSize != output.getSize())
    output = DataArray(newSize);

  output.clear();
  for (int z = 0; z < getSize().d; ++z) {
    for (int y = 0; y < getSize().h; ++y) {
      for (int x = 0; x < getSize().w; ++x) {
        output.at(x + width, y + width, z) = at(z, y, z);
      }
    }
  }
}

void DataArray::removeFrame(int width, DataArray & output) const {
  Size newSize = size;
  newSize.w -= 2 * width;
  newSize.h -= 2 * width;

  if (newSize.w <= 0 || newSize.h <= 0) {
    std::cerr << "..." << std::endl;
    return;
  }

  if (newSize != output.getSize())
    output = DataArray(newSize);

  for (int z = 0; z < newSize.d; ++z) {
    for (int y = 0; y < newSize.h; ++y) {
      for (int x = 0; x < newSize.w; ++x) {
        output.at(x, y, z) = at(x + width, y + width, z);
      }
    }
  }
}


Layer::~Layer() {}


ConvolutionalLayer::ConvolutionalLayer(DataArray::Size inputSize, int depth, int stride, int zeroPadding, int filterSize) :
  inputSize(inputSize), depth(depth), stride(stride), zeroPadding(zeroPadding), filterSize(filterSize) {

  inputSize.w += 2 * zeroPadding;
  inputSize.h += 2 * zeroPadding;

  int inW = inputSize.w - filterSize;
  int inH = inputSize.h - filterSize;

  if (inW < 0 || inW % stride != 0) std::cerr << "Неправильные параметры" << std::endl; // error
  if (inH < 0 || inH % stride != 0) std::cerr << "Неправильные параметры" << std::endl; // error

  outputSize.w = inW / stride + 1;
  outputSize.h = inH / stride + 1;
  outputSize.d = depth;

  for (int i = 0; i < depth; ++i) {
    filters.push_back( std::pair<DataArray, float>(DataArray(filterSize, filterSize, inputSize.d), 0) );
    filters.back().first.fillRnd();
  }
}


void ConvolutionalLayer::propagate(const DataArray & input, DataArray & output) const {
  if (input.getSize() != inputSize || output.getSize() != outputSize) {
    std::cerr << "Неправильные размеры входных или выходных данных" << std::endl;
    return;
  }

  for (int d = 0; d < depth; ++d) {
    const DataArray & filter = filters[d].first;

    for (int xo = 0; xo < outputSize.w; ++xo) {
      for (int yo = 0; yo < outputSize.h; ++yo) {
        float & val = output.at(xo, yo, d);
        val = 0;

        int ymin = yo * stride;
        int ymax = yo * stride + filterSize;

        int xmin = xo * stride;
        int xmax = xo * stride + filterSize;

        for (int z = 0; z < inputSize.d; ++z) {
          for (int y = ymin; y < ymax; ++y) {
            for (int x = xmin; x < xmax; ++x) {
              val += input.at(x, y, z) * filter.at(x - xmin, y - ymin, z);
            }
          }
        }

        val = func(val + filters[d].second);
      }
    }
  }
}


void ConvolutionalLayer::backPropagate(const DataArray & input, const DataArray & output, const DataArray & error, DataArray & inputError, float lambda) {
  if (inputError.getSize() != getInputSize() || input.getSize() != getInputSize()) {
    std::cerr << "Ошибка" << std::endl;
    return;
  }

  if (error.getSize() != getOutputSize() || output.getSize() != getOutputSize()) {
    std::cerr << "Ошибка" << std::endl;
    return;
  }

  for (int d = 0; d < depth; ++d) {
    DataArray & filter = filters[d].first;

    for (int xo = 0; xo < outputSize.w; ++xo) {
      for (int yo = 0; yo < outputSize.h; ++yo) {
        float err = error.at(xo, yo, d);
        float dr = output.at(xo, yo, d);
        float cf = err * lambda * (1 - dr) * dr;

        int ymin = yo * stride;
        int ymax = yo * stride + filterSize;

        int xmin = xo * stride;
        int xmax = xo * stride + filterSize;

        for (int z = 0; z < inputSize.d; ++z) {
          for (int y = ymin; y < ymax; ++y) {
            for (int x = xmin; x < xmax; ++x) {
              filter.at(x - xmin, y - ymin, z) -= cf * input.at(x, y, z);
            }
          }
        }

        filters[d].second -= cf;
      }
    }
  }
}



void MaxPoolLayer::propagate(const DataArray & input, DataArray & output) const {
  output.clear();
  // TODO Прочекать размеры и так далее
  for (int z = 0; z < outputSize.d; ++z) {
    for (int y = 0; y < outputSize.h; y += filterSize) {
      for (int x = 0; x < outputSize.w; x += filterSize) {
        float & res = output.at(x / filterSize, y / filterSize, z);
        res = input.at(x, y, z);
        for (int dy = 0; dy < filterSize; ++dy) {
          for (int dx = 0; dx < filterSize; ++dx) {
            res = std::max(res, input.at(x + dx, y + dy, z));
          }
        }
      }
    }
  }
}


void MaxPoolLayer::backPropagate(const DataArray & input, const DataArray & output, const DataArray & error, DataArray & inputError, float lambda) {
  inputError.clear();
  // TODO Прочекать размеры и так далее
  for (int z = 0; z < outputSize.d; ++z) {
    for (int y = 0; y < outputSize.h; y += filterSize) {
      for (int x = 0; x < outputSize.w; x += filterSize) {
        int ddx = 0, ddy = 0;
        float res = input.at(x, y, z);
        for (int dy = 0; dy < filterSize; ++dy) {
          for (int dx = 0; dx < filterSize; ++dx) {
            if (input.at(x + dx, y + dy, z) > res) {
              res = input.at(x + dx, y + dy, z);
              ddx = dx;
              ddy = dy;
            }
          }
        }

        inputError.at(x + ddx, y + ddy, z) = error.at(x / filterSize, y / filterSize, z);
      }
    }
  }
}