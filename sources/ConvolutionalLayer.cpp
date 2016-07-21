#include "ConvolutionalLayer.h"

DataArray::Size ConvolutionalLayer::getInputSize() const {
  return inputSize;
}


DataArray::Size ConvolutionalLayer::getOutputSize() const {
  return outputSize;
}


ConvolutionalLayer::ConvolutionalLayer(DataArray::Size inputSize, int depth, int stride, int zeroPadding, int filterSize) :
  inputSize(inputSize), depth(depth), stride(stride), zeroPadding(zeroPadding), filterSize(filterSize) {

  realInputSize = inputSize;

  realInputSize.w += 2 * zeroPadding;
  realInputSize.h += 2 * zeroPadding;

  realInput = DataArray(realInputSize);
  realInputError = DataArray(realInputSize);

  int inW = realInputSize.w - filterSize;
  int inH = realInputSize.h - filterSize;

  if (inW < 0 || inW % stride != 0) std::cerr << "Неправильные параметры" << std::endl; // error
  if (inH < 0 || inH % stride != 0) std::cerr << "Неправильные параметры" << std::endl; // error

  outputSize.w = inW / stride + 1;
  outputSize.h = inH / stride + 1;
  outputSize.d = depth;

  for (int i = 0; i < depth; ++i) {
    filters.push_back( std::pair<DataArray, float>(DataArray(filterSize, filterSize, inputSize.d), 0) );
    filters.back().first.fillRnd(0, 0.01);
    // filters.back().first.clear();
  }
}


void ConvolutionalLayer::propagate(const DataArray & input, DataArray & output) {
  if (input.getSize() != inputSize || output.getSize() != outputSize) {
    std::cerr << "Неправильные размеры входных или выходных данных" << std::endl;
    return;
  }
  
  input.addZeros(zeroPadding, realInput);

  for (int d = 0; d < depth; ++d) {
    const DataArray & filter = filters[d].first;

    for (int xo = 0; xo < outputSize.w; ++xo) {
      for (int yo = 0; yo < outputSize.h; ++yo) {
        float & val = output.at(xo, yo, d);
        val = 0;

        int ymin = yo * stride;
        int ymax = ymin + filterSize;

        int xmin = xo * stride;
        int xmax = xmin + filterSize;

        for (int z = 0; z < inputSize.d; ++z) {
          for (int y = ymin; y < ymax; ++y) {
            for (int x = xmin; x < xmax; ++x) {
              val += realInput.at(x, y, z) * filter.at(x - xmin, y - ymin, z);
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

  inputError.addZeros(zeroPadding, realInputError);
  input.addZeros(zeroPadding, realInput);

  for (int d = 0; d < depth; ++d) {
    DataArray & filter = filters[d].first;

    for (int xo = 0; xo < outputSize.w; ++xo) {
      for (int yo = 0; yo < outputSize.h; ++yo) {
        float err = error.at(xo, yo, d);
        float dr = output.at(xo, yo, d);
        float cf = err * (1 - dr) * dr;

        int ymin = yo * stride;
        int ymax = ymin + filterSize;

        int xmin = xo * stride;
        int xmax = xmin + filterSize;

        for (int z = 0; z < inputSize.d; ++z) {
          for (int y = ymin; y < ymax; ++y) {
            for (int x = xmin; x < xmax; ++x) {
              realInputError.at(x, y, z) += cf * filter.at(x - xmin, y - ymin, z);
            }
          }
        }
      }
    }
  }

  for (int d = 0; d < depth; ++d) {
    DataArray & filter = filters[d].first;

    for (int xo = 0; xo < outputSize.w; ++xo) {
      for (int yo = 0; yo < outputSize.h; ++yo) {
        float err = error.at(xo, yo, d);
        float dr = output.at(xo, yo, d);
        float cf = err * lambda * (1 - dr) * dr;

        // std::cout << err << " " << dr << " " << cf << std::endl;

        int ymin = yo * stride;
        int ymax = yo * stride + filterSize;

        int xmin = xo * stride;
        int xmax = xo * stride + filterSize;

        for (int z = 0; z < inputSize.d; ++z) {
          for (int y = ymin; y < ymax; ++y) {
            for (int x = xmin; x < xmax; ++x) {
              // if (z == 1 && y - ymin == 0 && x - xmin == 0) { std::cout << cf << " " << realInput.at(x, y, z) << std::endl; }
              filter.at(x - xmin, y - ymin, z) -= cf * realInput.at(x, y, z);
            }
          }
        }

        filters[d].second -= cf;
      }
    }
  }

  realInputError.removeFrame(zeroPadding, inputError);
}

