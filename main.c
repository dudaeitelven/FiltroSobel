#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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
	int nroProcessos;
	int iForImagem, jForImagem;
	int range, meio;
	int lacoJ, limiteJ;
	int lacoI, limiteI;
	int posMatriz;
	int MascaraX[3*3], MascaraY[3*3];
	int tamMatrizAux;
	int iForAux, jForAux;
	int xRed   ;
	int xGreen ;
	int xBlue  ;
	int yRed   ;
	int yGreen ;
	int yBlue  ;
	int posTeste;
	int iForSobel;
	int jForSobel;

	CABECALHO cabecalho;
	RGB *imagemEntrada, *imagemSaida;
	RGB *imagemX, *imagemY;
	
	if ( argc != 4){
		printf("%s <img_entrada> <img_saida> <processos> \n", argv[0]);
		exit(0);
	}

	entrada 		= argv[1];
	saida 			= argv[2];
    nroProcessos 	= atoi(argv[3]);

	FILE *fin = fopen(entrada, "rb");
	if ( fin == NULL ){
		printf("Erro ao abrir o arquivo entrada%s\n", entrada);
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
	imagemSaida  	= (RGB *)malloc(cabecalho.altura*cabecalho.largura*sizeof(RGB));
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
			posMatriz = iForImagem * cabecalho.largura + jForImagem;

			imagemSaida[posMatriz].red    = 0.2126 * imagemEntrada[posMatriz].red;
			imagemSaida[posMatriz].green  = 0.7152 * imagemEntrada[posMatriz].green;
			imagemSaida[posMatriz].blue   = 0.0722 * imagemEntrada[posMatriz].blue;		
		}
	}

	//Varrer os pixels da imagem
	for(iForImagem=0; iForImagem<cabecalho.altura; iForImagem++){
		for(jForImagem=0; jForImagem<cabecalho.largura; jForImagem++){
			xRed   = 0;
			xGreen = 0;
			xBlue  = 0;
			yRed   = 0;
			yGreen = 0;
			yBlue  = 0;

			iForSobel = ((iForImagem * cabecalho.largura) - 1);
			jForSobel = (jForImagem - 1);

			//Aplicar o filtro sobel no pixel
			for (iForAux=0;iForAux<3;iForAux++){
				for (jForAux=0;jForAux<3;jForAux++){
					if ((iForSobel >= 0) && (jForSobel >= 0)) {
						posTeste = (iForSobel + jForSobel);

						xRed   += imagemSaida[posTeste].red   * MascaraX[iForAux];
						xGreen += imagemSaida[posTeste].green * MascaraX[iForAux];
						xBlue  += imagemSaida[posTeste].blue  * MascaraX[iForAux];

						yRed   += imagemSaida[posTeste].red   * MascaraY[iForAux];
						yGreen += imagemSaida[posTeste].green * MascaraY[iForAux];
						yBlue  += imagemSaida[posTeste].blue  * MascaraY[iForAux];
					}

					jForSobel++;
				}

				iForSobel++;
				jForSobel = (jForImagem - 1);
			}

			posMatriz = (iForImagem * cabecalho.largura) + jForImagem;

			imagemSaida[posMatriz].red    = sqrt(pow(xRed,2)   + pow(yRed,2));
			imagemSaida[posMatriz].green  = sqrt(pow(xGreen,2) + pow(yGreen,2));
			imagemSaida[posMatriz].blue   = sqrt(pow(xBlue,2)  + pow(yBlue,2));

			//Gambia pra debugar
			// int r,g,b;
			// r = imagemSaida[posMatriz].red ;
			// g = imagemSaida[posMatriz].green ;
			// b = imagemSaida[posMatriz].blue ;
			// printf("%i, %i, %i\n",r,g,b);
		}
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

	fclose(fin);
	fclose(fout);

	printf("Arquivo %s gerado.\n", saida);
}