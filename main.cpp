// VERSION: v0.1 draft
// LICENSE: Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International
//          Commercial Use: Commercial organisations must contact Author for Commercial License.
//          See LICENSE.html for full license
//          https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode
// TITLE__: xSDOCconvert
// AUTHOR : xAuthor
// SOURCE : https://github.com/agiletimesoft/xSDOCconvert
//
// PURPOSE: converts text.dat to a windows Low Endian unicode format .txt file
//          (text.dat is in a Samsung Notes sdoc file)
//
// EXE USE: exename pathtext.dat pathout.txt
//          (after you have extracted the text.dat from sdoc)
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Line
{
	Line() 
	{
		clean();
	}
	void clean()
	{
		text =0;
		size =0;
	}
	wchar_t*text;
	int size;
	void swapendian()
	{
		for(int i =0; i < size; i++)
		{
			text[i] = (0xFF00 & text[i]) >> 8 | (0x00FF & text[i]) << 8;
		}
	}
};

class SDOC
{
public:
	SDOC() {indata = 0; insize =0; linecount =0; lines =0;}
	~SDOC() 
	{
		delete[] lines;
		delete[] indata;				
	}
	char*indata;
	int insize;
	unsigned int filetype;
	Line title;
	Line *lines;
	int linecount;
	bool fixup()
	{
		char*data = indata;

		filetype = SWAP32( *(unsigned int*)(data) );
		data+=4;
		title.size = SWAP32( *(unsigned int*)(data) );
		data+=4;

		unsigned int headerfourcc = 30;		
		if( filetype != headerfourcc)
			return false;
		
		title.text = (wchar_t*)(data);
		title.swapendian();		
		data+=(title.size*2);
		
		linecount = SWAP32( *(unsigned int*)(data) );
		data+=4;

		lines = new Line[linecount];
		
		bool foundend = false;
		for(int i =0; (data<(indata+insize)) && i < linecount; i++)
		{
			int linenumber = SWAP32( *(unsigned int*)(data) );
			data+=4;

			if(linenumber == 0x00464F45) 
			//note: when theres only 3 bytes left (EOF), my added null terminator byte allows a uint match, without reading outside allocated mem.
			//someone wanted to save 1byte in each file? i.e a 4 byte total file size field
			{
				foundend = true;
				break;
			}
									
			/*if(linenumber!=i)
			{
				printf("bad data. line number doesn't match line count %d\n", i);
				return false;				
			}
			if(lastlinefound!=(linenumber-1))
			{
				printf("bad data. out of sequence line number found at line %d\n", i);
				return false;				
			}
			lastlinefound = linenumber;*/

			lines[i].size = SWAP32( *(unsigned int*)(data) );
			data+=4;

			lines[i].text = (wchar_t*)data;
			lines[i].swapendian();			
			data+=(lines[i].size*2);			
			wprintf(lines[i].text);
			printf("\n");
		};
		
		return true;
	}
	void swapstring(wchar_t*t, int size)
	{
		for(int i =0; i < size; i++)
		{
			t[i] = (0xFF00 & t[i]) >> 8 | (0x00FF & t[i]) << 8;
		}
	}
	wchar_t SWAP16(wchar_t a)
	{ 
		return (0xFF00 & a) >> 8 |
				(0x00FF & a) << 8;		
	}

	unsigned int SWAP32(unsigned int a)
	{
		return (0xFF000000 & a) >> 24 |
				(0x00FF0000 & a) >> 8 |
				(0x0000FF00 & a) << 8 |
				(0x000000FF & a) << 24;		
	}

	bool readfile(char*filename)
	{
		title.clean();

		delete[] lines;
		lines=0;
		linecount=0;

		delete[] indata; 
		insize =0;
		
		bool ret = false;
		FILE *in = NULL;
		errno_t err = fopen_s(&in,filename,"rb");
		if(err != 0)
		{
			printf("cant open input file for read %s\n", filename);
			ret = false;	
		}
		else
		{
			fseek(in,0,SEEK_END);
			insize  = ftell(in);		
			if(!insize)
			{
				printf("empty sdoc file\n");				
			}
			else
			{
				indata = new char[insize+1];
				if(!indata)
				{
					printf("unable to allocate mem %d\n", insize+1);				
				}
				else
				{
					fseek(in,0,SEEK_SET);
					fread(indata, 1, insize, in);
					indata[insize]= 0;

					ret = fixup();
				}
			}
		
			fclose(in);			
		}
		
		return ret;
	}	
};

int main(int n, char*c[])
{
	if(n < 3)
	{
		printf("exe input.sdoc output.txt");	
		return 0;	
	}

	SDOC sdoc;
	if(sdoc.readfile(c[1]))
	{
		FILE *out = NULL;
		errno_t er= fopen_s(&out, c[2],"wb");
		if(er != 0)
		{
			printf("cant open output file for write %s\n", c[2]);
			return 0;	
		}
										
		for(int i =0; i < sdoc.linecount; i++)
		{			
			if( sdoc.lines[i].text )
			{
				fwrite( sdoc.lines[i].text, 1, (sdoc.lines[i].size)*2, out);		
				fputc( '\r', out);
				fputc( 0x00, out);
				fputc( '\n', out);
				fputc( 0x00, out);			
			}
		}
		fclose(out);
	
		printf("done");
	}
		
	return 0;
}