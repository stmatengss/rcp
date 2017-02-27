#include <stdio.h>
#include <fcntl.h>


int main(int argc, char **argv) {
		FILE *fp = fopen(argv[1], "r");
		fseek(fp, 0L, SEEK_END);
		int sz = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
		printf("size: %d\n", sz);
		fclose(fp);
		return 0;
}
