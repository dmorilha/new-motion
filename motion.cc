#include <array>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include <cassert>

using Image = std::vector<uint8_t>;

struct Context {
  struct Node {
    Image image;
    /* timestamp */
  };
  std::deque<Node> list_;
  void push_frame(Image && frame);
};

void Context::push_frame(Image && frame) {
  list_.emplace_back(Node{
      .image = std::move(frame),
      });
}

struct AbstractMotion {
  /*
   * The way the motion detection works on GNU Motion is
   * by returning an integer that represents the difference
   * between a reference frame and a new frame.
   *
   * This difference is calculated based on how many pixels
   * changed between the two frames.
   */
  virtual uint64_t detect_motion(Context &) = 0;
  virtual void acquire_frame(Image & frame) = 0;
  virtual void action() { };
};

struct MyMotion : AbstractMotion {
  int frame_number_ = 0;
  /* pixel threshold */
  const int pixel_threshold = 216;
  void acquire_frame(Image & frame) override;
  uint64_t detect_motion(Context &) override;
};

uint64_t MyMotion::detect_motion(Context & context) {
  uint8_t difference_score = 0;
  uint64_t different_pixel_count = 0;

  const Image & reference = context.list_.front().image,
    & latest = context.list_.back().image;

  assert(reference.size() == latest.size());

  /* Grayscale represented as RGB 8 */
  for (int i = 0; i < reference.size(); i += 3) {
    std::array<unsigned char, 3> pixel_a, pixel_b, difference;

    pixel_a[0] = reference[i];
    pixel_a[1] = reference[i + 1];
    pixel_a[2] = reference[i + 2];

    pixel_b[0] = latest[i];
    pixel_b[1] = latest[i + 1];
    pixel_b[2] = latest[i + 2];

    if (pixel_a[0] != pixel_b[0]
        || pixel_a[1] != pixel_b[1]
        || pixel_a[2] != pixel_b[2]) {
      difference[0] = std::abs(pixel_a[0] - pixel_b[0]);
      difference[1] = std::abs(pixel_a[1] - pixel_b[1]);
      difference[2] = std::abs(pixel_a[2] - pixel_b[2]);
      if (pixel_threshold <= (difference[0] + difference[1] + difference[2]) / 3) {
        ++different_pixel_count;
      }
    }
  }

  return different_pixel_count;
}

void MyMotion::acquire_frame(Image & frame) {
  std::array<char, 3> buffer;
  std::fstream file;
  std::stringstream file_name;
  file_name << "./frames/frame-" << std::setw(4) << std::setfill('0') << (frame_number_ + 1) << ".yuv";
  std::cerr << "file name " << file_name.str() << std::endl;
  file.open(file_name.str());
  file.seekg(0x8a); /* skip bitmap header */
  std::size_t read = file.readsome(buffer.data(), buffer.size());
  while (0 < read) {
    while (buffer.size() > read) {
      read += file.readsome(buffer.data() + read, buffer.size() - read);
    }
    assert(3 == read);
    frame.insert(frame.end(), buffer.begin(), buffer.end());
    read = file.readsome(buffer.data(), buffer.size());
  }
  frame_number_ = (frame_number_ + 1) % 465;
}

int main() {
  std::unique_ptr<AbstractMotion> motion = std::make_unique<MyMotion>();
  Context context;

  {
    Image frame = Image();
    motion->acquire_frame(frame);
    context.push_frame(std::move(frame));
  }

  for (int i = 1; 1000 > i; ++i) {
    Image frame = Image();
    motion->acquire_frame(frame);
#if 0
    context.push_frame(std::move(frame));
    const uint64_t motion_detected = motion->detect_motion(context);
    if (0 < motion_detected) {
      std::cout << "motion detected " << motion_detected << std::endl;
    }
#endif
  }
  return 0;
}
