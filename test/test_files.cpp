#include "catch.hpp"
#include "exceptions.h"
#include "file_table.h"
#include "files.h"

#include <algorithm>
#include <errno.h>
#include <set>
#include <string.h>
#include <vector>

TEST_CASE("File table")
{
    using namespace securefs;
    auto base_dir = OSService::temp_name("tmp/file_table", ".dir");
    OSService::get_default().ensure_directory(base_dir, 0755);

    key_type master_key;
    id_type null_id, file_id;
    memset(master_key.data(), 0xFF, master_key.size());
    memset(null_id.data(), 0, null_id.size());
    securefs::generate_random(file_id.data(), file_id.size());
    const char* xattr_name = "com.apple.FinderInfo...";
    const securefs::PODArray<char, 32> xattr_value(0x11);

    {
        auto root = std::make_shared<OSService>(base_dir);
        FileTable table(2, root, master_key, 0, 3000, 16);
        auto dir = dynamic_cast<Directory*>(table.create_as(null_id, FileBase::DIRECTORY));
        table.create_as(file_id, FileBase::REGULAR_FILE);
        dir->add_entry(".", null_id, FileBase::DIRECTORY);
        dir->add_entry("..", null_id, FileBase::DIRECTORY);
        dir->add_entry("hello", file_id, FileBase::REGULAR_FILE);
        try
        {
            dir->setxattr(xattr_name, xattr_value.data(), xattr_value.size(), 0);
        }
        catch (const securefs::ExceptionBase& e)
        {
            REQUIRE(e.error_number() == ENOTSUP);
        }
        table.close(dir);
    }

    {
        auto all_ids = find_all_ids(base_dir.c_str());
        REQUIRE(all_ids.size() == 2);
        REQUIRE(all_ids.find(null_id) != all_ids.end());
        REQUIRE(all_ids.find(file_id) != all_ids.end());
    }

    {
        auto root = std::make_shared<OSService>(base_dir);
        FileTable table(2, root, master_key, 0, 3000, 16);
        auto dir = dynamic_cast<Directory*>(table.open_as(null_id, FileBase::DIRECTORY));
        securefs::PODArray<char, 32> xattr_test_value(0);
        try
        {
            dir->getxattr(xattr_name, xattr_test_value.data(), xattr_test_value.size());
            REQUIRE(xattr_value == xattr_test_value);
        }
        catch (const securefs::ExceptionBase& e)
        {
            REQUIRE(e.error_number() == ENOTSUP);
        }

        std::set<std::string> filenames;
        dir->iterate_over_entries([&](const std::string& fn, const id_type&, int) {
            filenames.insert(fn);
            return true;
        });
        REQUIRE((filenames == decltype(filenames){".", "..", "hello"}));
        id_type id;
        int type;
        dir->get_entry("hello", id, type);
        REQUIRE(memcmp(id.data(), file_id.data(), id.size()) == 0);
        bool is_regular_file = type == FileBase::REGULAR_FILE;
        REQUIRE(is_regular_file);
        table.close(dir);
    }
}
