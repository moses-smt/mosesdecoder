#include "util/tempfile.hh"

#include <fstream>

#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE TempFileTest
#include <boost/test/unit_test.hpp>

namespace util
{
namespace
{

BOOST_AUTO_TEST_CASE(temp_dir_has_path)
{
  BOOST_CHECK(temp_dir().path().size() > 0);
}

BOOST_AUTO_TEST_CASE(temp_dir_creates_temp_directory)
{
  const temp_dir t;
  BOOST_CHECK(boost::filesystem::exists(t.path()));
  BOOST_CHECK(boost::filesystem::is_directory(t.path()));
}

BOOST_AUTO_TEST_CASE(temp_dir_creates_unique_directory)
{
  BOOST_CHECK(temp_dir().path() != temp_dir().path());
}

BOOST_AUTO_TEST_CASE(temp_dir_cleans_up_directory)
{
  std::string path;
  {
    const temp_dir t;
    path = t.path();
  }
  BOOST_CHECK(!boost::filesystem::exists(path));
}

BOOST_AUTO_TEST_CASE(temp_dir_cleanup_succeeds_if_directory_contains_file)
{
  std::string path;
  {
    const temp_dir t;
    path = t.path();
    boost::filesystem::create_directory(path + "/directory");
    std::ofstream file((path + "/file").c_str());
    file << "Text";
    file.flush();
  }
  BOOST_CHECK(!boost::filesystem::exists(path));
}

BOOST_AUTO_TEST_CASE(temp_dir_cleanup_succeeds_if_directory_is_gone)
{
  std::string path;
  {
    const temp_dir t;
    path = t.path();
    boost::filesystem::remove_all(path);
  }
  BOOST_CHECK(!boost::filesystem::exists(path));
}

BOOST_AUTO_TEST_CASE(temp_file_has_path)
{
  BOOST_CHECK(temp_file().path().size() > 0);
}

BOOST_AUTO_TEST_CASE(temp_file_creates_temp_file)
{
  const temp_file f;
  BOOST_CHECK(boost::filesystem::exists(f.path()));
  BOOST_CHECK(boost::filesystem::is_regular_file(f.path()));
}

BOOST_AUTO_TEST_CASE(temp_file_creates_unique_file)
{
  BOOST_CHECK(temp_file().path() != temp_file().path());
}

BOOST_AUTO_TEST_CASE(temp_file_creates_writable_file)
{
  const std::string data = "Test-data-goes-here";
  const temp_file f;
  std::ofstream outfile(f.path().c_str());
  outfile << data;
  outfile.flush();
  std::string read_data;
  std::ifstream infile(f.path().c_str());
  infile >> read_data;
  BOOST_CHECK_EQUAL(data, read_data);
}

BOOST_AUTO_TEST_CASE(temp_file_cleans_up_file)
{
  std::string path;
  {
    const temp_file f;
    path = f.path();
  }
  BOOST_CHECK(!boost::filesystem::exists(path));
}

BOOST_AUTO_TEST_CASE(temp_file_cleanup_succeeds_if_file_is_gone)
{
  std::string path;
  {
    const temp_file t;
    path = t.path();
    boost::filesystem::remove(path);
  }
  BOOST_CHECK(!boost::filesystem::exists(path));
}

} // namespace anonymous
} // namespace util
