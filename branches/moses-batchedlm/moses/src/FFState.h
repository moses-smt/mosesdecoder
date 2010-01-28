namespace Moses {

class FFState {
 public:
  virtual ~FFState();
  virtual int Compare(const FFState& other) const = 0;
};

}
