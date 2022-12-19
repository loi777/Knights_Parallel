run : purge all
	@echo "iniciando executavel:\n\n\n"
	@./passeio_cavalo

orig : purge __original
	@echo "testando original:\n\n\n"
	@./passeio_cavalo

full : purge
	@gcc -o3 -Wall passeio_cavalo_original.c -o passeio_cavalo_orig
	@gcc -o3 -Wall -fopenmp passeio_cavalo.c -o passeio_cavalo
	@echo "\n\nPARALELO\n\n" >> results.txt
	@./passeio_cavalo >> results.txt
	@echo "\n\nORIGINAL\n\n" >> results.txt
	@./passeio_cavalo_orig >> results.txt
	@echo "Terminado de calcular ambos metodos"

#--

all : passeio_cavalo

#--

purge : clean
	@clear
	@rm -f passeio_cavalo passeio_cavalo_orig
	@echo "purge terminado"

clean :
	@echo "nao tem clean"

#--

passeio_cavalo : passeio_cavalo.c
	@gcc -o3 -Wall -fopenmp passeio_cavalo.c -o passeio_cavalo

__original : passeio_cavalo_original.c
	@gcc -o3 -Wall passeio_cavalo_original.c -o passeio_cavalo_orig