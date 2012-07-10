#ifndef moses_FeatureExtractor_h
#define moses_FeatureExtractor_h


struct FeatureTypes
{
  bool m_sourceExternal;
  bool m_sourceInternal;
  bool m_targetInternal;
  bool m_paired;

  size_t m_contextWindow;
}

// vector of words, each word is a vector of factors (form|lemma|POS for now)
typedef std::vector<std::vector<std::string> > ContextType; 
typedef std::map<std::string, size_t> TargetIndexType;

class FeatureExtractor
{
public:
	FeatureExtractor(FeatureTypes ft, FeatureConsumer &fc, const TargetIndexType &targetIndex, bool train);

	void GenerateFeatures(const ContextType &context,
                  size_t spanStart,
                  size_t spanEnd,
                  const vector<size_t> &translations
                  vector<float> &losses);

  {
    for (translation in translations) {
      if (train) {
        m_fc->Train(name, losses[i]);
      } else {
        losses[i] = m_fc->Predict(name);
      }
    }
  }

private:
  bool m_train;
  FeatureTypes m_ft;
  const TargetIndexType &m_targetIndex;
};

#endif // moses_FeatureExtractor_h
