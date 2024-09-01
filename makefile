all: make_M make_S make_H make_L make_c

make_c: client.c
	gcc -o client client.c

make_M: serverM.c
	gcc -o serverM serverM.c

make_S: serverS.c                     
	gcc -o serverS serverS.c 

make_H: serverH.c                          
	gcc -o serverH serverH.c

make_L: serverL.c    
	gcc -o serverL serverL.c 

clean:
	rm -f client serverM serverH serverS serverL
