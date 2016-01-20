#ifndef __STRINGUTILS_H__
#define __STRINGUTILS_H__

namespace stdex {
	typedef std::basic_string<TCHAR> tstring;
	typedef std::basic_stringstream<TCHAR> tstringstream; 

	template <typename Char>
	std::basic_string<Char>& escape(std::basic_string<Char>& str, Char c, Char escChar)
	{
		size_t i = str.find(c);

		if(i == std::string::npos)
			return str;

		do { 
			str.insert(i, 1, escChar);
			i = str.find(c, i + 2); // Advance two (one for the escape character and one for the escaped character)
		} while(i != std::string::npos);

		return str;
	}
}

#endif // __STRINGUTILS_H__