char* concat(const char* s1, const char* s2) {
	size_t l1 = strlen(s1);
	size_t l2 = strlen(s2);

	char* out = (char*) malloc(l1 + l2 + 1);
	memcpy(out, s1, l1);
	memcpy(out + l1, s2, l2 + 1); // incl. terminal null

	return out;
}