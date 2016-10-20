#include "utf8_streambuf.hpp"
int main() {
	std::wcout.imbue(std::locale(""));
	u8ostreambuf buf1{ std::wcout.rdbuf() };
	u8istreambuf buf2{ std::wcin.rdbuf() };
	auto tmp1 = std::cout.rdbuf(&buf1);
	auto tmp2 = std::cin.rdbuf(&buf2);
	int a;
	std::cin >> a;
	std::cout
		<< a << std::endl
		<< u8"ありきたりな世界" << std::endl;
	std::cout << u8"𠮷野家" << std::endl;//CP932に変換できないのでVSのstd::wcoutの実装ではこの一行まるごと出力されない
}
