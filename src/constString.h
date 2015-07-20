#ifndef CONSTSTRING_H
#define CONSTSTRING_H

#include <cstdio>
#include <stdint.h>

class constString {
	const char *buffer;
	size_t length;
 public:
	constString() { buffer = 0; length = 0; }
	constString(const char *s);
	constString(const char *s, size_t n);
	operator bool()   { return  length>0 &&  buffer; }
	bool empty()      { return length<=0 || !buffer; }
	bool operator !() { return length<=0 || !buffer; }
	size_t len() const { return length; }
	// ojito, este método NO añade el \0 al final
	operator const char *() const { return buffer; }

	char operator [] (int i) const { return buffer[i]; }
	char operator [] (unsigned int i) const { return buffer[i]; }
	// char operator [] (size_t i) const { return buffer[i]; }

	char *newString() const;

	// el método siguiente crea una cadena nueva que se debe
	// liberar posteriormente con delete[]

	// por implementar:
	// char* operator char*() { return buffer; } // conversor
	bool operator == (const constString &otro) const;
	bool operator != (const constString &otro) const;
	bool operator <  (const constString &otro) const;
	bool operator <= (const constString &otro) const;
	bool operator >  (const constString &otro) const;
	bool operator >= (const constString &otro) const;
	bool operator == (const char *otro) const { return operator==(constString(otro)); }
	bool operator != (const char *otro) const { return operator!=(constString(otro)); }
	bool operator <  (const char *otro) const { return operator<(constString(otro)); }
	bool operator <= (const char *otro) const { return operator<=(constString(otro)); }
	bool operator >  (const char *otro) const { return operator>(constString(otro)); }
	bool operator >= (const char *otro) const { return operator>=(constString(otro)); }
 	void skip(size_t n);
	void ltrim(const char *acepta = " \t");
// 	void rtrim();
// 	void trim();
	bool is_prefix(const constString &theprefix);
	bool is_prefix(const char *theprefix);
	bool skip(const char *theprefix);
	bool skip(const constString &theprefix);
	constString extract_prefix(size_t prefix_len);
	constString extract_token(const char *separadores=" \t,;\r\n");
	constString extract_line();
	constString extract_u_line();
	bool extract_char(char *resul);
	bool extract_int(int *resul, int base = 10, 
			 const char *separadores=" \t,;\r\n");
	bool extract_long(long int *resul, int base = 10, 
			  const char *separadores=" \t,;\r\n");
	bool extract_long_long(long long int *resul, int base = 10, 
			       const char *separadores=" \t,;\r\n");
	bool extract_float(float *resul, 
			   const char *separadores=" \t,;\r\n");
	bool extract_double(double *resul, 
			    const char *separadores=" \t,;\r\n");
	bool extract_float_binary(float *resul);
	bool extract_double_binary(double *resul);

	void print(FILE *F=stdout);
};

char *copystr(const char *c);

#endif // CONSTSTRING_H
