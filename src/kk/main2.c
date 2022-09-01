#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int main(int argc, char** argv){



int width=0, height=0, channels;

		//stbi_set_flip_vertically_on_load(1);
		unsigned char* image = stbi_load(argv[1], &width, &height, &channels, 3);
		
		//printf("%i \n", width);
		//printf("%i \n", height);


		if(channels != 3){
			printf("%i \n", width);
			printf("%c \n", image);
			printf("%i \n",channels);
			exit(-1);
		}

	
		else{
			int k=0;
			/*
			for(int i = 0; i < width*height*2; i+=4) {
				printf("0x%x%x%x%x, ", image[i], image[i+1], image[i+2], image[i+3]);			
				k+=1;
			}
			*/
			
			
			uint16_t* image2= (uint16_t*)image;
			
			for (int i=0; i<width*height;i++){
				printf("0x%x, ", image2[i]);
				k++;
			}
			
			
			
			printf("%i\n",k);
				
		}
}
