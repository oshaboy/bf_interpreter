#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
const char * help="\
-f [file]\n\
-b [8/16/32] : interpreter cell is n bits\n\
-c [num] : use n interpreter cells\n\
-h : you literally just used it";
enum Bitness{
	EIGHT_BIT=1,
	SIXTEEN_BIT=2,
	THIRTY_TWO_BIT=4
};
typedef struct {
	long * free_cells;
	FILE * stream;
	int clobbered_cells;
	int current_cell;
	enum Bitness bits : 8;
	bool is_stream_a_file : 1;
} Settings;
typedef struct {
    char * s;
    size_t n;
    size_t capacity;
} String;

void append(String * str, char * s){
    size_t len=strlen(s);
    if (str->capacity == 0){
        str->capacity=16;
        str->s=malloc(str->capacity);
    }

    while (str->n+len > str->capacity){
        str->capacity*=4;
        str->s=realloc(str->s,str->capacity);
    }
    strcpy(str->s+str->n, s);
    str->n+=len;
}
void append_n(String * str, char * pos, char * neg, long n){
	if (n<0){
		for (long i=0; i<-n; i++){
			append(str, neg);
		}
	} else if (n>0) {
		for (long i=0; i<n; i++){
			append(str, pos);
		}
	}
}
void initialize_settings(Settings * settings){
	if (settings->bits==0)
		settings->bits=EIGHT_BIT;
	
	if (settings->clobbered_cells==0)
		settings->clobbered_cells=4;
	
	if (settings->free_cells==NULL) 
		settings->free_cells=calloc(settings->clobbered_cells,sizeof(long));
	

	if (settings->stream == NULL){
		settings->stream=stdin;
		settings->is_stream_a_file=false;
	}
}
static div_t div_nearest(long a, long b){
	div_t result=div(a,b);
	if (result.rem>(b/2)){
		result.quot++;
		result.rem-=b;
	} else if (result.rem<(-b/2)){
		result.quot--;
		result.rem+=b;
	}
	return result;
}
String to_brainfuck_datum(Settings * settings, long datum, String buf){
    long difference=LONG_MAX;
    int closest_cell_index;
	for (int i=0; i<settings->clobbered_cells; i++){
        long curdiff=datum-settings->free_cells[i];
	    if (labs(difference)>labs(curdiff)){
            difference=curdiff;  
            closest_cell_index=i;
        }
	}
    int distance=closest_cell_index-settings->current_cell;
	append_n(&buf, ">>","<<",distance);
    settings->current_cell=closest_cell_index;

	div_t difference_16=div_nearest(difference, 16);
	if (difference_16.quot < 2 && difference_16.quot > -2)
		append_n(&buf, "+","-",difference);
	else {
		append(&buf,">");
		append_n(&buf, "+","+", difference_16.quot);
		append(&buf,
			difference_16.quot<0?
			"[<---------------->-]<":
			"[<++++++++++++++++>-]<"
		);
		append_n(&buf, "+","-",difference_16.rem);
	}

	append(&buf,".");
    settings->free_cells[settings->current_cell]+=difference;
	return buf;
}
char * to_brainfuck_stream(Settings * settings){
	long datum=0;
	String buf={0};
	append_n(&buf, "[-]>[-]>",NULL, settings->clobbered_cells);
	append_n(&buf, "<<", NULL, settings->clobbered_cells);
	while (!feof(settings->stream)){
		fread(&datum, settings->bits, 1, settings->stream);
		if (!feof(settings->stream))
			buf=to_brainfuck_datum(settings,datum,buf);
		
	}
	return buf.s;
	
}
int main(int argc, char * argv[]){
	Settings settings={0};
	initialize_settings(&settings);
	for (int i=1; i<argc; i+=2){
		if (argv[i][0]=='-'){
			switch(argv[i][1]){
				case 'c':
					settings.clobbered_cells=atoi(argv[i+1]);
					if (settings.clobbered_cells <= 0)
						settings.clobbered_cells=4;
					
					break;

				case 'b':
					switch(atoi(argv[i+1])){
						case 8:
							settings.bits=EIGHT_BIT;
							break;
						case 16:
							settings.bits=SIXTEEN_BIT;
							break;
						case 32:
							settings.bits=THIRTY_TWO_BIT;
							break;
					}
					break;
				
				case 'f':
					settings.stream=fopen(argv[i+1],"rb");
					if (settings.stream)
						settings.is_stream_a_file=true;
					else 
						exit(1);
					break;
				case 'h':
					printf("%s\n",help);
					exit(0);
				default:
					exit(1);

			}
		} else {
			exit(1);
		}
	}
	char * code=to_brainfuck_stream(&settings);
	printf("%s",code);
	free(code);
	if (settings.is_stream_a_file) fclose(settings.stream);
	free(settings.free_cells);
	return 0;
}