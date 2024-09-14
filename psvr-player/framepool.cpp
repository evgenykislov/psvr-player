#include "framepool.h"

#include <cassert>
#include <mutex>
#include <vector>

std::vector<Frame> frame_pool_;
std::mutex pool_lock_;


Frame RequestFrame(int align_width, int align_height) {
  std::lock_guard<std::mutex> lk(pool_lock_);
  while (!frame_pool_.empty()) {
    Frame fr = std::move(frame_pool_.back());
    frame_pool_.pop_back();
    int fr_width, fr_height;
    fr.GetSizes(&fr_width, &fr_height, nullptr, nullptr);
    if (fr_width == align_width && fr_height == align_height) {
      return fr;
    }
  }

  return Frame(align_width, align_height);
}


void ReleaseFrame(Frame&& frame) {
  std::lock_guard<std::mutex> lk(pool_lock_);
  frame_pool_.push_back(std::move(frame));
}


Frame::Frame(int align_width, int align_height) {
  if (align_width <= 0 || align_height <= 0) {
    throw std::logic_error("align_sizes below zero or zero");
  }
  size_t s = align_width * align_height * kPixelSize;
  data_.resize(s);
  align_height_ = align_height;
  align_width_ = align_width;
  width_ = 0;
  height_ = 0;
}


Frame::Frame(Frame&& arg) {
  Move(std::move(arg));
}


Frame& Frame::operator=(Frame&& arg) {
  if (&arg == this) {
    return *this;
  }

  Move(std::move(arg));
  return *this;
}


void Frame::SetSize(int width, int height) {
  if (width > align_width_ || height > align_height_) {
    throw std::runtime_error("SetSize uses oversizes");
  }

  width_ = width;
  height_ = height;
}


void Frame::GetSizes(int* width, int* height, int* align_width, int* align_height) {
  if (width) { *width = width_; }
  if (height) { *height = height_; }
  if (align_width) { *align_width = align_width_; }
  if (align_height) { *align_height = align_height_; }
}


void* Frame::GetData(size_t& data_size) {
  data_size = data_.size();
  return data_.data();
}


void Frame::Move(Frame&& arg) {
  assert(&arg != this);
  std::swap(data_, arg.data_);
  std::swap(width_, arg.width_);
  std::swap(height_, arg.height_);
  std::swap(align_width_, arg.align_width_);
  std::swap(align_height_, arg.align_height_);
}
