#include "FullConnectedLayer.h"

FullConnectedLayer::FullConnectedLayer(const ConvolutionalLayer & c) : Base(c) {}

FullConnectedLayer::FullConnectedLayer(DataArray::Size inputSize, int depth)
    : Base(inputSize, depth, inputSize.w, 0, inputSize.w) {}

void FullConnectedLayer::write(std::ostream & stream) const {
  Base::write(stream);
}

FullConnectedLayer* FullConnectedLayer::read(std::istream & stream) {
  return new FullConnectedLayer(*Base::read(stream));
}

Layer::LayerType FullConnectedLayer::getType() const {
  return Layer::FULL_CONNECTED;
}
