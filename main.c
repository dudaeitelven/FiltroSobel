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
3 - Digitar para rodar: ./main borboleta.bmp saida.bmp 3
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
	int iPosLinha, jPosColuna;
	int pid, id_seq;
	int shmid, chave = 5;

	CABECALHO cabecalho;
	RGB *imagemEntrada, *imagemSaida;
	RGB *imagemX, *imagemY;
	RGB *imagemSaidaTeste;
	
	if ( argc != 4){
		//printf("%s <img_entrada> <img_saida> <num_proc> \n", argv[0]);
		exit(0);
	}

	entrada = argv[1];
	saida 	= argv[2];
    np 		= atoi(argv[3]);

	FILE *fin = fopen(entrada, "rb");
	if ( fin == NULL ){
		//printf("Erro ao abrir o arquivo entrada%s\n", entrada);
		exit(0);
	}

	FILE *fout = fopen(saida, "wb");
	if ( fout == NULL ){
		//printf("Erro ao abrir o arquivo saida %s\n", saida);
		exit(0);
	}

	//Ler cabecalho entrada
	fread(&cabecalho, sizeof(CABECALHO), 1, fin);	

	//Alocar imagems
	imagemEntrada   = (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));
	imagemSaida  	= (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));
	imagemX  		= (RGB *)malloc((3*3)*sizeof(RGB));
	imagemY  		= (RGB *)malloc((3*3)*sizeof(RGB));
	
    shmid = shmget(chave, cabecalho.altura*cabecalho.largura*sizeof(RGB), 0600 | IPC_CREAT);
	imagemSaidaTeste = (RGB *)shmat(shmid, 0, 0);

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
	MascaraY[0] =  1; //P1
	MascaraY[1] =  2; //P2
	MascaraY[2] =  1; //P3
	MascaraY[3] =  0; //P4
	MascaraY[4] =  0; //P5 - Central
	MascaraY[5] =  0; //P6
	MascaraY[6] = -1; //P7
	MascaraY[7] = -2; //P8
	MascaraY[8] = -1; //P9

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

	//Transformar em escala de cinza
	for(iForImagem=0; iForImagem<cabecalho.altura; iForImagem++){
		for(jForImagem=0; jForImagem<cabecalho.largura; jForImagem++){
			iPosMatriz = iForImagem * cabecalho.largura + jForImagem;

			imagemSaida[iPosMatriz].red    = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
			imagemSaida[iPosMatriz].green  = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
			imagemSaida[iPosMatriz].blue   = (0.2126 * imagemEntrada[iPosMatriz].red) + (0.7152 * imagemEntrada[iPosMatriz].green) + (0.0722 * imagemEntrada[iPosMatriz].blue);
		}
	}

	int iPosLinhaAnt, iPosLinhaPrx;
	int jPosColunaAnt, jPosColunaPrx;

	if (1 == 1){
		//Varrer os pixels da imagem
		for(iForImagem=0; iForImagem<cabecalho.altura; iForImagem++){
			for(jForImagem=0; jForImagem<cabecalho.largura; jForImagem++){
				iPosLinhaAnt  = (iForImagem-1) * cabecalho.largura;
				jPosColunaAnt = jForImagem-1;

				iPosLinha  = (iForImagem) * cabecalho.largura;
				jPosColuna = jForImagem;

				iPosLinhaPrx  = (iForImagem+1) * cabecalho.largura;
				jPosColunaPrx = jForImagem+1;
				
				valorX = 0;
				valorY = 0;

				//Mascaras
				if ((iForImagem-1 >=0) && (iForImagem-1 <=399)) {
					if ((jPosColunaAnt >=0) && (jPosColunaAnt <=599)) {
						valorX   =  MascaraX[0] * imagemSaida[(iPosLinhaAnt) + (jPosColunaAnt)].red;
						valorY   =  MascaraY[0] * imagemSaida[(iPosLinhaAnt) + (jPosColunaAnt)].red;
						//printf("%i, %i - %i\n",iForImagem-1,jPosColunaAnt,(iPosLinhaAnt) + (jPosColunaAnt));
					}
				} 
				if ((iForImagem-1 >=0) && (iForImagem-1 <=399)) {
					valorX   += MascaraX[1] * imagemSaida[(iPosLinhaAnt) + (jPosColuna)].red;
					valorY   += MascaraY[1] * imagemSaida[(iPosLinhaAnt) + (jPosColuna)].red;
					//printf("%i, %i - %i\n",iForImagem-1,jPosColuna,(iPosLinhaAnt) + (jPosColuna));
				} 
				if ((iForImagem-1 >=0) && (iForImagem-1 <=399)) {
					if ((jPosColunaPrx >=0) && (jPosColunaPrx <=599)) {
						valorX   += MascaraX[2] * imagemSaida[(iPosLinhaAnt) + (jPosColunaPrx)].red;
						valorY   += MascaraY[2] * imagemSaida[(iPosLinhaAnt) + (jPosColunaPrx)].red;
						//printf("%i, %i - %i\n",iForImagem-1,jPosColunaPrx,(iPosLinhaAnt) + (jPosColunaPrx));
					}
				}
				
				if ((jPosColunaAnt >=0) && (jPosColunaAnt <=599)) {
					valorX   += MascaraX[3] * imagemSaida[(iPosLinha) + (jPosColunaAnt)].red; 
					valorY   += MascaraX[3] * imagemSaida[(iPosLinha) + (jPosColunaAnt)].red; 
					//printf("%i, %i - %i\n",iForImagem,jPosColunaAnt,(iPosLinha) + (jPosColunaAnt));
				}
				valorX   += MascaraX[4] * imagemSaida[(iPosLinha) + (jPosColuna)].red;
				//printf("%i, %i - %i\n",iForImagem,jPosColuna,(iPosLinha) + (jPosColuna)); 
				if ((jPosColunaPrx >=0) && (jPosColunaPrx <=599)) {
					valorX   += MascaraX[5] * imagemSaida[(iPosLinha) + (jPosColunaPrx)].red;
					valorY   += MascaraY[5] * imagemSaida[(iPosLinha) + (jPosColunaPrx)].red;
					//printf("%i, %i - %i\n",iForImagem,jPosColunaPrx,(iPosLinha) + (jPosColunaPrx));
				}
				
				if ((iForImagem+1 >=0) && (iForImagem+1 <=399)) {
					if ((jPosColunaAnt >=0) && (jPosColunaAnt <=599)) {
						valorX   += MascaraX[6] * imagemSaida[(iPosLinhaPrx) + (jPosColunaAnt)].red;
						valorY   += MascaraY[6] * imagemSaida[(iPosLinhaPrx) + (jPosColunaAnt)].red;
						//printf("%i, %i - %i\n",iForImagem+1,jPosColunaAnt,(iPosLinhaPrx) + (jPosColunaAnt));
					}
				} 
				if ((iForImagem+1 >=0) && (iForImagem+1 <=399)) {
					valorX   += MascaraX[7] * imagemSaida[(iPosLinhaPrx) + (jPosColuna)].red; 
					valorY   += MascaraY[7] * imagemSaida[(iPosLinhaPrx) + (jPosColuna)].red; 
					//printf("%i, %i - %i\n",iForImagem+1,jPosColuna, (iPosLinhaPrx) + (jPosColuna));
				}
				if ((iForImagem+1 >=0) && (iForImagem+1 <=399)) {
					if ((jPosColunaPrx >=0) && (jPosColunaPrx <=599)) {
						valorX   += MascaraX[8] * imagemSaida[(iPosLinhaPrx) + (jPosColunaPrx)].red; 
						valorY   += MascaraY[8] * imagemSaida[(iPosLinhaPrx) + (jPosColunaPrx)].red;
						////printf("%i, %i - %i\n",iForImagem+1,jPosColunaPrx,(iPosLinhaPrx) + (jPosColunaPrx));
					}
				}

				//Imagem de saida		
				iPosMatriz = iPosLinha + jForImagem;

				imagemSaida[iPosMatriz].red    = sqrt(pow(valorX,2) + pow(valorY,2));
				imagemSaida[iPosMatriz].green  = sqrt(pow(valorX,2) + pow(valorY,2));
				imagemSaida[iPosMatriz].blue   = sqrt(pow(valorX,2) + pow(valorY,2));
			}
		}
	}

	//Escrever arquivo saida
	if (1 == 1) {
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
	}

	fclose(fin);
	fclose(fout);

	free(imagemEntrada);
    free(imagemSaida);
	free(imagemX);
    free(imagemY);

	//printf("Arquivo %s gerado.\n", saida);
}