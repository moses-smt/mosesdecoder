#include "BaseManager.h"
#include "StaticData.h"
#include "moses/FF/StatelessFeatureFunction.h"
#include "moses/FF/StatefulFeatureFunction.h"
#include "moses/TranslationTask.h"

#include <vector>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/filesystem.hpp>

using namespace std;

namespace Moses
{

BaseManager::BaseManager(ttasksptr const& ttask)
  : m_ttask(ttask), m_source(*(ttask->GetSource().get()))
{ }

const InputType&
BaseManager::GetSource() const
{
  return m_source;
}

void
BaseManager::
OutputSearchGraphAsHypergraph(std::ostream& out) const
{
  // This virtual function that may not be implemented everywhere, but it should for
  // derived classes that use it
  UTIL_THROW2("Not implemented.");
}

void 
BaseManager::
OutputSearchGraphAsHypergraph(std::string const& fname, size_t const precision) const
{
  std::string odir = boost::filesystem::path(fname).parent_path().string();
  if (! boost::filesystem::exists(odir))
    boost::filesystem::create_directory(odir);
  UTIL_THROW_IF2(!boost::filesystem::is_directory(odir),
		 "Cannot output hypergraphs to " << odir 
		 << " because that path exists but is not a directory.");

  // not clear why we need to output the weights every time we dump a search 
  // graph into a file again, but that's what the old code did.
  
  string weightsFile = odir + "/weights";
  TRACE_ERR("The weights file is " << weightsFile << "\n");
  ofstream weightsOut;
  weightsOut.open(weightsFile.c_str());
  weightsOut.setf(std::ios::fixed);
  weightsOut.precision(6);
  // just temporarily, till we've implemented weight scoring in the manager
  // (or the translation task)
  StaticData::Instance().GetAllWeights().Save(weightsOut);
  weightsOut.close();
  
  boost::iostreams::filtering_ostream file;
  if (boost::ends_with(fname, ".gz"))
    file.push(boost::iostreams::gzip_compressor());
  else if (boost::ends_with(fname, ".bz2"))
    file.push( boost::iostreams::bzip2_compressor() );
  file.push( boost::iostreams::file_sink(fname, ios_base::out) );
  if (file.is_complete() && file.good()) 
    {
      file.setf(std::ios::fixed);
      file.precision(precision);
      this->OutputSearchGraphAsHypergraph(file);
      file.flush();
    }
  else 
    {
      TRACE_ERR("Cannot output hypergraph for line " 
		<< this->GetSource().GetTranslationId()
		<< " because the output file " << fname
		<< " is not open or not ready for writing"
		<< std::endl);
    }
  file.pop();
}




/***
 * print surface factor only for the given phrase
 */
void BaseManager::OutputSurface(std::ostream &out, const Phrase &phrase,
                                const std::vector<FactorType> &outputFactorOrder,
                                bool reportAllFactors) const
{
  UTIL_THROW_IF2(outputFactorOrder.size() == 0,
                 "Cannot be empty phrase");
  if (reportAllFactors == true) {
    out << phrase;
  } else {
    size_t size = phrase.GetSize();
    for (size_t pos = 0 ; pos < size ; pos++) {
      const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[0]);
      out << *factor;
      UTIL_THROW_IF2(factor == NULL,
                     "Empty factor 0 at position " << pos);

      for (size_t i = 1 ; i < outputFactorOrder.size() ; i++) {
        const Factor *factor = phrase.GetFactor(pos, outputFactorOrder[i]);
        UTIL_THROW_IF2(factor == NULL,
                       "Empty factor " << i << " at position " << pos);

        out << "|" << *factor;
      }
      out << " ";
    }
  }
}

// Emulates the old operator<<(ostream &, const DottedRule &) function.  The
// output format is a bit odd (reverse order and double spacing between symbols)
// but there are scripts and tools that expect the output of -T to look like
// that.
void BaseManager::WriteApplicationContext(std::ostream &out,
    const ApplicationContext &context) const
{
  assert(!context.empty());
  ApplicationContext::const_reverse_iterator p = context.rbegin();
  while (true) {
    out << p->second << "=" << p->first << " ";
    if (++p == context.rend()) {
      break;
    }
    out << " ";
  }
}

} // namespace


