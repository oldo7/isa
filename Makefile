dns:
	gcc dns.c -o dns

test:
	gcc dns.c -o dns
	gcc test1.c -o test1
	./test1