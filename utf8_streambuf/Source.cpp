#include "utf8_streambuf.hpp"
int main() {
	std::wcout.imbue(std::locale(""));
	utf8_streambuf buf1{ std::wcout.rdbuf() };
	utf8_streambuf buf2{ std::wcin.rdbuf() };
	auto tmp1 = std::cout.rdbuf(&buf1);
	auto tmp2 = std::cin.rdbuf(&buf2);
	int a;
	std::cin >> a;
	std::cout << u8"ありきたりな世界" << std::endl;
}
