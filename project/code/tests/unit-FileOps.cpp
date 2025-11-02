#include "MattDaemon.hpp"

static std::string nowstr() {
	char buf[32];
	std::time_t t = std::time(NULL);
	std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", std::localtime(&t));
	return buf;
}

static void print_result(const std::string& name, const std::string& expected, const std::string& obtained, bool ok) {
	std::cout << GRY1 "[TEST] " << name << RST
				<< GRY2 "\nEXPECTED: " RST << expected
				<< GRY2 "\nOBTAINED: " RST << obtained
				<< "\nRESULT  : " << (ok ? LIME "PASS" : REDD "FAIL") << RST "\n\n";
}

int test_FileOps() {
	std::cout << CYAN "=================== FileOps ===================" RST "\n";

	std::string base = "/tmp/md_fileops_test_" + nowstr();
	bool ok;
	int ret = 0;

	ok = FileOps::ensureDir(base, 0755);
	print_result("ensureDir(base)", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	std::string sub = base + "/sub/dir";
	ok = FileOps::ensureDir(sub, 0755);
	print_result("ensureDir(sub/dir)", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	ok = FileOps::exists(base);
	print_result("exists(base)", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	std::vector<std::string> listing;
	ok = FileOps::listFiles(base, listing);
	print_result("listFiles(base) returns >=1", "true", (ok && !listing.empty()) ? "true" : "false", ok && !listing.empty());
	if (!ok) ret++;

	std::string f1 = base + "/log.txt";
	FileOps fops;
	ok = fops.bind(f1, true);
	print_result("bind(f1, append=true)", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	ok = fops.writeLine("hello");
	print_result("writeLine('hello')", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	ok = fops.flush();
	print_result("flush()", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	uint64_t sz = 0;
	ok = FileOps::sizeOf(f1, sz);
	print_result("sizeOf(f1) > 0", "true", (ok && sz > 0) ? "true" : "false", ok && sz > 0);
	if (!ok) ret++;

	std::string content;
	ok = fops.readAll(content);
	print_result("readAll(f1) == 'hello\\n'", "true", (ok && content == "hello\n") ? "true" : ("false (\"" + content + "\")"), ok && content == "hello\n");
	if (!ok) ret++;

	ok = fops.write(std::string("world"));
	print_result("write('world')", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	ok = fops.write("\n", 1);
	print_result("write('\\n',1)", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	ok = fops.readAll(content);
	print_result("readAll(f1) == 'hello\\nworld\\n'", "true", (ok && content == "hello\nworld\n") ? "true" : ("false (\"" + content + "\")"), ok && content == "hello\nworld\n");
	if (!ok) ret++;

	ok = fops.reopenAppend();
	print_result("reopenAppend()", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	std::string beforeReopen = content;
	ok = fops.readAll(content);
	bool same = ok && content == beforeReopen;
	print_result("content unchanged after reopenAppend()", "true", same ? "true" : ("false (\"" + content + "\")"), same);
	if (!ok) ret++;

	ok = fops.writeLine("after-reopen");
	print_result("writeLine('after-reopen')", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	ok = fops.readAll(content);
	bool endsOk = ok && content.find("after-reopen\n") != std::string::npos;
	print_result("readAll contains 'after-reopen\\n'", "true", endsOk ? "true" : ("false (\"" + content + "\")"), endsOk);
	if (!ok) ret++;

	std::string f2 = base + "/renamed.txt";
	ok = FileOps::renameFile(f1, f2);
	print_result("renameFile(f1->f2)", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	ok = FileOps::exists(f2);
	print_result("exists(f2)", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	ok = FileOps::removeFile(f2);
	print_result("removeFile(f2)", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	ok = !FileOps::exists(f2);
	print_result("exists(f2)==false", "true", ok ? "true" : "false", ok);
	if (!ok) ret++;

	std::string envp = base + "/env.test";
	{
		FileOps tmp;
		tmp.bind(envp, false);
		tmp.writeLine("# comment");
		tmp.writeLine("KEY1=VAL1");
		tmp.writeLine("KEY2 =  VAL 2");
		tmp.writeLine("EMPTY=");
		tmp.unbind();
	}
	std::unordered_map<std::string,std::string> kv;
	ok = FileOps::readKeyValuesFile(envp, kv);
	bool kvOk = ok && kv["KEY1"] == "VAL1" && kv["KEY2"] == "VAL 2";
	print_result("readKeyValuesFile basic keys", "true", kvOk ? "true" : "false", kvOk);
	if (!kvOk) ret++;

	int fd = FileOps::openFd(base + "/lock.lck", O_RDWR | O_CREAT, 0644);
	bool lockOk = (fd >= 0) && FileOps::lockExclusiveNonBlock(fd);
	print_result("lockExclusiveNonBlock(fd)", "true", lockOk ? "true" : "false", lockOk);
	if (!lockOk) ret++;
	FileOps::closeFd(fd);

	std::cout << GRY1 "[CLEANUP] Removing test directory tree under " << base << RST "\n";
	std::vector<std::string> files;
	if (FileOps::listFiles(base, files)) {
		for (size_t i = 0; i < files.size(); ++i) {
			std::string p = base + "/" + files[i];
			FileOps::removeFile(p);
		}
	}
	rmdir((base + "/sub/dir").c_str());
	rmdir((base + "/sub").c_str());
	rmdir(base.c_str());

	std::cout << CYAN "===============================================" RST "\n";

	return ret;
}
