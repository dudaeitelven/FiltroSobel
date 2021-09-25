#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <unistd.h>

#pragma pack(1)

/*
Para compilar:
1 - Abrir o local do fonte
2 - Digitar para compilar: gcc main.c -o main -lm
3 - Digitar para rodar: ./main borboleta.bmp saida.bmp 4
*/

struct cabecalho {
	unsigned short tipo;
	unsigned int tamanho_arquivo;
	unsigned short reservado1;
	unsigned short reservado2;
	unsigned int offset;
	unsigned int tamanho_image_header;
	int largura;
	int altura;
	unsigned short planos;
	unsigned short bits_por_pixel;
	unsigned int compressao;
	unsigned int tamanho_imagem;
	int largura_resolucao;
	int altura_resolucao;
	unsigned int numero_cores;
	unsigned int cores_importantes;
};
typedef struct cabecalho CABECALHO;

struct rgb{
	unsigned char blue;
	unsigned char green;
	unsigned char red;
};
typedef struct rgb RGB;

int main(int argc, char **argv ){
	char *entrada, *saida;
	char ali, aux;
	int np;
	int iForImagem, jForImagem;
	int range, meio;
	int lacoJ, limiteJ;
	int lacoI, limiteI;
	int iPosMatriz;
	int MascaraX[3*3], MascaraY[3*3];
	int valorX   ;
	int valorY   ;
	int iPosLinhaAnt, jPosColunaAnt;
	int iPosLinha, jPosColuna;
	int iPosLinhaPrx, jPosColunaPrx;

	int pid, id_seq;
	int shmid, chave = 5;
	int i;

	CABECALHO cabecalho;
	RGB *imagemEntrada, *imagemSaida;
	RGB *imagemX, *imagemY;
	RGB *imagemCinza;
	RGB *imagemCompartilhada;
	
	if ( argc != 4){
		printf("%s <img_entrada> <img_saida> <num_proc> \n", argv[0]);
		exit(0);
	}

	entrada = argv[1];
	saida 	= argv[2];
    np 		= atoi(argv[3]);

	FILE *fin = fopen(entrada, "rb");
	if ( fin == NULL ){
		printf("Erro ao abrir o arquivo entrada %s\n", entrada);
		exit(0);
	}

	FILE *fout = fopen(saida, "wb");
	if ( fout == NULL ){
		printf("Erro ao abrir o arquivo saida %s\n", saida);
		exit(0);
	}

	//Ler cabecalho entrada
	fread(&cabecalho, sizeof(CABECALHO), 1, fin);	

	//Alocar imagems
	imagemEntrada   = (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));
	//imagemSaida  	= (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));
	imagemCinza  	= (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));
	imagemX  		= (RGB *)malloc((3*3)*sizeof(RGB));
	imagemY  		= (RGB *)malloc((3*3)*sizeof(RGB));

	//MascaraX
	MascaraX[0] = -1; //P1
	MascaraX[1] =  0; //P2
	MascaraX[2] =  1; //P3
	MascaraX[3] = -2; //P4
	MascaraX[4] =  0; //P5 - Central
	MascaraX[5] =  2; //P6
	MascaraX[6] = -1; //P7
	MascaraX[7] =  0; //P8
	MascaraX[8] =  1; //P9

	//MascaraY
	MascaraY[0] = -1; //P1
	MascaraY[1] = -2; //P2
	MascaraY[2] = -1; //P3
	MascaraY[3] =  0; //P4
	MascaraY[4] =  0; //P5 - Central
	MascaraY[5] =  0; //P6
	MascaraY[6] =  1; //P7
	MascaraY[7] =  2; //P8
	MascaraY[8] =  1; //P9
	
	//Leitura da imagem entrada
	for(iForImagem=0; iForImagem<cabecalho.altura; iForImagem++){
		ali = (cabecalho.largura * 3) % 4;

		if (ali != 0){
			ali = 4 - ali;
		}

		for(jForImagem=0; jForImagem<cabecalho.largura; jForImagem++){
			fread(&imagemEntrada[iForImagem * cabecalho.largura + jForImagem], sizeof(RGB), 1, fin);
		}

		for(jForImagem=0; jForImagem<ali; jForImagem++){
			fread(&aux, sizeof(unsigned char), 1, fin);
		}
	}

	//Compartilhar memoria
    shmid = shmget(chave, cabecalho.altura*cabecalho.largura*sizeof(RGB), 0600 | IPC_CREAT);
	if ((shmget(chave, cabecalho.altura*cabecalho.largura*sizeof(RGB), 0600 | IPC_CREAT)) < 0) {
		printf("Erro no compartilhamento da memoria \n");
		exit(0);
	}

	//Alocar memoria compartilhada
	imagemSaida = (RGB *)shmat(shmid, 0, 0);
	if (((RGB *)shmat(shmid, 0, 0)) < 0) {
		printf("Erro ao alocar memoria compartilhada\n");
		exit(0);
	}

	//Fork
	id_seq = 0;
    for(i=1; i<np;i++){
        pid = fork();
        if ( pid == 0){
            id_seq = i;
            break;
        } 
    }

	//Transformar em escala de cinza
	for(iForImagem=0; iForImagem<cabecalho.altura; iForImagem++){
		for(jForImagem=0; jForImagem<cabecalho.largura; jForImagem++){
			iPosMatriz = iForImagem * cabecalho.largura + jForImagem;

			imagemCinza[iPosMatriz].red    = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
			imagemCinza[iPosMatriz].green  = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
			imagemCinza[iPosMatriz].blue   = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
		}
	}

	//Varrer os pixels da imagem
	for(iForImagem=1; iForImagem<(cabecalho.altura-1); iForImagem++){
		for(jForImagem=1; jForImagem<(cabecalho.largura-1); jForImagem++){
			//Calcular as posicoes
			iPosLinhaAnt  = (iForImagem-1) * cabecalho.largura;
			jPosColunaAnt = jForImagem-1;

			iPosLinha  = (iForImagem) * cabecalho.largura;
			jPosColuna = jForImagem;

			iPosLinhaPrx  = (iForImagem+1) * cabecalho.largura;
			jPosColunaPrx = jForImagem+1;

			//Mascaras
			valorX   = MascaraX[0] * imagemCinza[(iPosLinhaAnt) + (jPosColunaAnt)].red
						+ MascaraX[1] * imagemCinza[(iPosLinhaAnt) + (jPosColuna)].red
						+ MascaraX[2] * imagemCinza[(iPosLinhaAnt) + (jPosColunaPrx)].red
						+ MascaraX[3] * imagemCinza[(iPosLinha)    + (jPosColunaAnt)].red
						+ MascaraX[4] * imagemCinza[(iPosLinha)    + (jPosColuna)].red
						+ MascaraX[5] * imagemCinza[(iPosLinha)    + (jPosColunaPrx)].red
						+ MascaraX[6] * imagemCinza[(iPosLinhaPrx) + (jPosColunaAnt)].red
						+ MascaraX[7] * imagemCinza[(iPosLinhaPrx) + (jPosColuna)].red
						+ MascaraX[8] * imagemCinza[(iPosLinhaPrx) + (jPosColunaPrx)].red; 

			valorY   = MascaraY[0] * imagemCinza[(iPosLinhaAnt) + (jPosColunaAnt)].red
						+ MascaraY[1] * imagemCinza[(iPosLinhaAnt) + (jPosColuna)].red
						+ MascaraY[2] * imagemCinza[(iPosLinhaAnt) + (jPosColunaPrx)].red
						+ MascaraY[3] * imagemCinza[(iPosLinha)    + (jPosColunaAnt)].red
						+ MascaraY[4] * imagemCinza[(iPosLinha)    + (jPosColuna)].red
						+ MascaraY[5] * imagemCinza[(iPosLinha)    + (jPosColunaPrx)].red
						+ MascaraY[6] * imagemCinza[(iPosLinhaPrx) + (jPosColunaAnt)].red
						+ MascaraY[7] * imagemCinza[(iPosLinhaPrx) + (jPosColuna)].red
						+ MascaraY[8] * imagemCinza[(iPosLinhaPrx) + (jPosColunaPrx)].red;
	
			//Imagem de saida		
			iPosMatriz = iPosLinha + jForImagem;

			imagemSaida[iPosMatriz].red    = sqrt(pow(valorX,2) + pow(valorY,2));
			imagemSaida[iPosMatriz].green  = sqrt(pow(valorX,2) + pow(valorY,2));
			imagemSaida[iPosMatriz].blue   = sqrt(pow(valorX,2) + pow(valorY,2));
		}
	}
	
	//Escrever arquivo saida
	if (id_seq == 0) {
		for(i=1; i<np;i++){
            wait(NULL);
        }    

		//Escrever cabecalho saida
		fwrite(&cabecalho, sizeof(CABECALHO), 1, fout);

		//Escrever a imagem saida
		for(iForImagem=0; iForImagem<cabecalho.altura; iForImagem++){
			ali = (cabecalho.largura * 3) % 4;

			if (ali != 0){
				ali = 4 - ali;
			}

			for(jForImagem=0; jForImagem<cabecalho.largura; jForImagem++){
				fwrite(&imagemSaida[iForImagem * cabecalho.largura + jForImagem], sizeof(RGB), 1, fout);
			}

			for(jForImagem=0; jForImagem<ali; jForImagem++){
				fwrite(&aux, sizeof(unsigned char), 1, fout);
			}
		}

		//Limpar compartilhamento
		shmdt(imagemSaida);
		shmctl(shmid, IPC_RMID, 0);

		printf("Arquivo %s gerado.\n", saida);
	}
	else {
		shmdt(imagemSaida);
	}

	fclose(fin);
	fclose(fout);
}
