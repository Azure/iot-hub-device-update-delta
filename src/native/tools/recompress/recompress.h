#include <language_support/include_filesystem.h>

void recompress(archive_diff::io::reader &reader, std::shared_ptr<archive_diff::io::writer> &writer);
void recompress(fs::path &source, fs::path &dest);
fs::path recompress_using_reader(fs::path &source, fs::path &dest);
std::string get_filehash_string(fs::path &path);
bool compare_file_hashes(fs::path &file1, fs::path &file2, const std::string &hash1, const std::string &hash2);
