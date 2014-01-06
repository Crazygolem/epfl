#include <sstream>

string concat(string s1, string s2) {
	std::ostringstream buff;
	buff << s1 << s2;
	return buff.str();
}

string toString(unsigned int i) {
	std::ostringstream conv;
	conv << i;
	return conv.str();
}