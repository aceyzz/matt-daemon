#include "MattDaemon.hpp"

struct TestContext {
	std::string base;
	std::string sub;
	std::string f1;
	std::string f2;
	FileOps fops;
	std::vector<std::string> listing;
	std::string content;
	std::string beforeReopen;
};

int test_FileOps() {
	std::cout << CYAN "=================== FileOps ===================" RST "\n";

	TestContext ctx;
	ctx.base = "/tmp/md_fileops_test_" + nowstr();
	ctx.sub  = ctx.base + "/sub/dir";
	ctx.f1   = ctx.base + "/log.txt";
	ctx.f2   = ctx.base + "/renamed.txt";

	Runner R;
	bool ok;

	ok = FileOps::ensureDir(ctx.base, 0755);
	R.expectTrue("ensureDir(base)", ok);

	ok = FileOps::ensureDir(ctx.sub, 0755);
	R.expectTrue("ensureDir(sub/dir)", ok);

	ok = FileOps::exists(ctx.base);
	R.expectTrue("exists(base)", ok);

	ctx.listing.clear();
	ok = FileOps::listFiles(ctx.base, ctx.listing);
	R.expectTrue("listFiles(base) returns >=1", ok && !ctx.listing.empty());

	ok = ctx.fops.bind(ctx.f1, true);
	R.expectTrue("bind(f1, append=true)", ok);

	ok = ctx.fops.writeLine("hello");
	R.expectTrue("writeLine('hello')", ok);

	ok = ctx.fops.flush();
	R.expectTrue("flush()", ok);

	uint64_t sz = 0;
	ok = FileOps::sizeOf(ctx.f1, sz);
	R.expectTrue("sizeOf(f1) > 0", ok && sz > 0);

	ok = ctx.fops.readAll(ctx.content);
	R.expectEq("readAll(f1) == 'hello\\n'", "true", ctx.content, ok && ctx.content == "hello\n");

	ok = ctx.fops.write(std::string("world"));
	R.expectTrue("write('world')", ok);

	ok = ctx.fops.write("\n", 1);
	R.expectTrue("write('\\n',1)", ok);

	ok = ctx.fops.readAll(ctx.content);
	R.expectEq("readAll(f1) == 'hello\\nworld\\n'", "true", ctx.content, ok && ctx.content == "hello\nworld\n");

	ctx.beforeReopen = ctx.content;
	ok = ctx.fops.reopenAppend();
	R.expectTrue("reopenAppend()", ok);

	ok = ctx.fops.readAll(ctx.content);
	R.expectEq("content unchanged after reopenAppend()", "true", ctx.content, ok && ctx.content == ctx.beforeReopen);

	ok = ctx.fops.writeLine("after-reopen");
	R.expectTrue("writeLine('after-reopen')", ok);

	ok = ctx.fops.readAll(ctx.content);
	R.expectEq("readAll contains 'after-reopen\\n'", "true", ctx.content, ok && ctx.content.find("after-reopen\n") != std::string::npos);

	ok = FileOps::renameFile(ctx.f1, ctx.f2);
	R.expectTrue("renameFile(f1->f2)", ok);

	ok = FileOps::exists(ctx.f2);
	R.expectTrue("exists(f2)", ok);

	ok = FileOps::removeFile(ctx.f2);
	R.expectTrue("removeFile(f2)", ok);

	ok = !FileOps::exists(ctx.f2);
	R.expectTrue("exists(f2)==false", ok);

	int fd = FileOps::openFd(ctx.base + "/lock.lck", O_RDWR | O_CREAT, 0644);
	bool lockOk = (fd >= 0) && FileOps::lockExclusiveNonBlock(fd);
	R.expectTrue("lockExclusiveNonBlock(fd)", lockOk);
	if (fd >= 0) FileOps::closeFd(fd);

	std::cout << GRY1 "[CLEANUP] Removing test directory tree under " << ctx.base << RST "\n";
	std::vector<std::string> files;
	if (FileOps::listFiles(ctx.base, files)) {
		for (size_t i = 0; i < files.size(); ++i) {
			FileOps::removeFile(ctx.base + "/" + files[i]);
		}
	}
	rmdir((ctx.base + "/sub/dir").c_str());
	rmdir((ctx.base + "/sub").c_str());
	rmdir(ctx.base.c_str());

	std::cout << CYAN "===============================================" RST "\n";
	final_result(R.failingList, R.failures, 20);
	std::cout << CYAN "===============================================" RST "\n";
	return R.failures;
}
