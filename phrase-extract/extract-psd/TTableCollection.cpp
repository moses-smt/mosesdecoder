#include "TTableCollection.h"
#include "Util.h"
#include <algorithm>

using namespace std;
using namespace PSD;

TTableCollection::TTableCollection(const string &ttableArg)
{
  m_targetIndex = new IndexType();

  // parse specification of translation tables
  if (ttableArg.find(':') != string::npos) {
    vector<string> ttableDefs = Moses::TokenizeMultiCharSeparator(ttableArg, ":::");
    vector<string>::const_iterator it;
    for (it = ttableDefs.begin(); it != ttableDefs.end(); it++) {
      vector<string> tokens = Moses::Tokenize(*it, ":");
      if (tokens.size() != 2) {
        cerr << "Wrong ttable definition. Use either '/path/to/table' or 'id1:path1:::id2:path2...'" << endl;
        exit(1);
      }
      TTableInfo info = { tokens[0], new TranslationTable(tokens[1], m_targetIndex) };
      m_ttables.push_back(info);
    }
  } else {
    // only 1 table, ID is not mandatory (backward compatibility)
    TTableInfo info = { "", new TranslationTable(ttableArg, m_targetIndex) };
    m_ttables.push_back(info);
  }
}

std::vector<PSD::Translation> TTableCollection::GetAllTranslations(const std::string &srcPhrase, bool intersection)
{
  map<size_t, size_t> allIDs; // allIDs[translationID] = number of phrase tables where translationID occurs
  vector<TTableInfo>::const_iterator tableIt;
  map<string, map<size_t, TTableTranslation> > ttableTranslations;

  // collect translations into maps indexed by target phrase ID, gather all translation IDs
  for (tableIt = m_ttables.begin(); tableIt != m_ttables.end(); tableIt++) {
    if (tableIt->m_ttable->SrcExists(srcPhrase)) {
      map<size_t, TTableTranslation> translations = tableIt->m_ttable->GetTranslations(srcPhrase);
      map<size_t, TTableTranslation>::const_iterator it;
      for (it = translations.begin(); it != translations.end(); it++)
        allIDs[it->first]++;
      ttableTranslations.insert(make_pair(tableIt->m_id, translations));
    }
  }

  // go over all translation IDs, over all phrase tables, create Translation entries
  vector<Translation> out;
  map<size_t, size_t>::const_iterator idIt;
  for (idIt = allIDs.begin(); idIt != allIDs.end(); idIt++) {
    Translation outTran;
    outTran.m_index = idIt->first;

    // if user wants phrase-table intersection, only return translations which occur in all phrase-tables
    if (intersection && idIt->second != m_ttables.size())
      continue;

    for (tableIt = m_ttables.begin(); tableIt != m_ttables.end(); tableIt++) {
      TTableEntry entry;
      entry.m_id = tableIt->m_id;
      if (tableIt->m_ttable->SrcExists(srcPhrase)) {
        map<size_t, TTableTranslation>::iterator it;
        // XXX inefficient, a multipath merge should be implemented here
        it = ttableTranslations[tableIt->m_id].find(outTran.m_index);
        if (it != ttableTranslations[tableIt->m_id].end()) {
          // this phrase table knows source phrase and translation *idIt
          entry.m_exists = true;
          entry.m_scores = it->second.m_scores;
          outTran.m_alignment = it->second.m_alignment;
        } else {
          entry.m_exists = false; // target phrase *idIt not in table *tableIt
        }
      } else {
        entry.m_exists = false; // source phrase is not in phrase table *tableIt
      }
      outTran.m_ttableScores.push_back(entry);
    }
    out.push_back(outTran);
  }

  return out;
}

bool TTableCollection::SrcExists(const string &srcPhrase)
{
  bool exists = false;
  vector<TTableInfo>::const_iterator tableIt;
  for (tableIt = m_ttables.begin(); tableIt != m_ttables.end(); tableIt++)
    exists |= tableIt->m_ttable->SrcExists(srcPhrase);
  return exists;
}

size_t TTableCollection::GetTgtPhraseID(const string &phrase, /* out */ bool *found)
{
  *found = false;
  IndexType::left_map::const_iterator it = m_targetIndex->left.find(phrase);
  if (it != m_targetIndex->left.end()) {
    *found = true;
    return it->second;
  } else {
    return 0; // user must test value of found!
  }
}

