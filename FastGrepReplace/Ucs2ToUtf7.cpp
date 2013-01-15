#include <string>

static inline char encode_char(unsigned char c) {
	if (c <= 0x19)					return c + 'A';
	if (c >= 0x1a && c <= 0x33)		return c - 0x1a + 'a';
	if (c >= 0x34 && c <= 0x3d)		return c - 0x34 + '0';
	if (c == 0x3e)					return '+';
	if (c == 0x3f)					return '/';
    return '=';
}

static inline void append(std::string &dist, char c) {
	if (c != '=') {
		dist.append(1, c);
	}
}

static inline std::string encode(std::string &src) {
	std::string dest = "";

	for (std::string::iterator it = src.begin(); it != src.end(); it += 3) {
		unsigned char c = (unsigned char)*it;
		append(dest, encode_char(c >> 2));
		if ((it + 1) == src.end()) {
			append(dest, encode_char(((c & 0x3) << 4)));
			break;
		}
		unsigned char c1 = (unsigned char)*(it + 1);
		append(dest, encode_char(((c & 0x3) << 4) + ((c1 >> 4))));
		if ((it + 2) == src.end()) {
			append(dest, encode_char((c1 & 0xf) << 2));
			break;
		}
		unsigned char c2 = (unsigned char)*(it + 2);
		append(dest, encode_char(((c1 & 0xf) << 2) + (c2 >> 6)));
		append(dest, encode_char(c2 & 0x3f));
	}
	return dest;
}

std::string UTF7_encode(std::wstring &src) {
	std::string dest = "";

	for (std::wstring::iterator it = src.begin(); ; ) {
		if (it == src.end()) {
			break;
		}
		wchar_t c = (wchar_t)*it;
		if (c == '+') {
			dest.append("+-");
		} else if (c > 0x7f) {
			std::string mb;
			mb.append(1, (char)(c >> 8));
			mb.append(1, (char)(c & 0xFF));
			for (it++; it != src.end(); it++) {
				wchar_t c2 = (wchar_t)*it;
				if (c2 > 0x7f) {
					mb.append(1, (char)(c2 >> 8));
					mb.append(1, (char)(c2 & 0xFF));
				} else {
					it--;
					break;
				}
			}
			std::string enc = encode(mb);
			dest.append(1, '+');
			dest.append(enc);
			dest.append(1, '-');
		} else {
			dest.append(1, (char)(c & 0xff));
		}
		if (it != src.end()) {
			it++;
		}
	}
	return dest;
}
