#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

typedef uint8_t Byte;
char *filePath;
Byte header[16];
Byte ProcesserStatus;
uint16_t PC;
struct{
    bool flag_Carry;
    bool flag_Zero;
    bool flag_InterruptDisable;
    bool flag_Decimal;
    bool flag_Overflow;
    bool flag_Negative;
}SP;
Byte A;
Byte X;
Byte Y;
Byte ram[0x800];
Byte rom[0x8000];
Byte vram[0x800];
bool cpuRunning;
Byte readByte(uint16_t Address){
    if(Address<0x800){
        return ram[Address];
    }
    if(Address<0x1fff){
        return ram[Address%0x800];
    }
    if(Address>=0x8000){
        return rom[Address-0x8000];
    }
}
uint16_t merge(Byte high,Byte low){
    return (high<<8)|low;
}
void writeByte(uint16_t Address,Byte Value){
    if(Address<0x800){
        ram[Address]=Value;
    }
    if(Address<0x1fff){
        ram[Address%0x800]=Value;
    }
    
}
void Reset(){
    
    FILE *fp = fopen(filePath,"rb");
    
    if(fp == NULL){
        printf("File does not exist :(\n");
        exit(1);
    }
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    SP.flag_InterruptDisable=1;
    Byte *totalRom = malloc(sizeof(Byte)*size) ;
    fread(totalRom,sizeof(Byte),size,fp);
    memcpy(header,totalRom,16);
    memcpy(rom,&totalRom[16],size-16);
    
    Byte PCLow = readByte(0xFFFC);
    Byte PCHigh = readByte(0xFFFD);
    printf("found PCHIGH:0x%02x, PCLOW:0x%02x",PCHigh,PCLow);
    PC=merge(PCHigh,PCLow);
}
void RunCPU(){
    Byte opCode = readByte(PC);
    PC++;
    int cycles=0;
    printf("running %02x \n",opCode);
    switch(opCode){
        case 0x02://halt
            cpuRunning = false;
            break;
        case 0x10:{//BPL
            Byte temp = readByte(PC);
            PC++;
            if(!SP.flag_Negative){
                int signedVal = temp;
                if(signedVal>127){
                    signedVal-=256;
                }
                int tempPC=PC+signedVal;
                if((tempPC&0xf000)!=(PC&0xf000)){
                    cycles=4;
                } else{
                    cycles=3;
                }
                PC=tempPC;
            } else{
                cycles=2;
            }
            break;
        }
        case 0x30:{//BMI
            Byte temp = readByte(PC);
            PC++;
            if(SP.flag_Negative){
                int signedVal = temp;
                if(signedVal>127){
                    signedVal-=256;
                }
                int tempPC=PC+signedVal;
                if((tempPC&0xf000)!=(PC&0xf000)){
                    cycles=4;
                } else{
                    cycles=3;
                }
                PC=tempPC;
            } else{
                cycles=2;
            }
            break;
        }
        case 0x50:{//BVC
            Byte temp = readByte(PC);
            PC++;
            if(!SP.flag_Overflow){
                int signedVal = temp;
                if(signedVal>127){
                    signedVal-=256;
                }
                int tempPC=PC+signedVal;
                if((tempPC&0xf000)!=(PC&0xf000)){
                    cycles=4;
                } else{
                    cycles=3;
                }
                PC=tempPC;
            } else{
                cycles=2;
            }
            break;
        }
        case 0x70:{//BVS
            Byte temp = readByte(PC);
            PC++;
            if(SP.flag_Overflow){
                int signedVal = temp;
                if(signedVal>127){
                    signedVal-=256;
                }
                int tempPC=PC+signedVal;
                if((tempPC&0xf000)!=(PC&0xf000)){
                    cycles=4;
                } else{
                    cycles=3;
                }
                PC=tempPC;
            } else{
                cycles=2;
            }
            break;
        }
        case 0x84://STY Zero Page
            printf("writing 0x%02x to 0x%02x\n",A,readByte(PC));
            writeByte(readByte(PC),Y);
            PC++;
            cycles=3;
            break;
        case 0x85://STA Zero Page
            printf("writing 0x%02x to 0x%02x\n",A,readByte(PC));
            writeByte(readByte(PC),A);
            PC++;
            cycles=3;
            break;
        case 0x86:{//STX Zero Page
            printf("writing 0x%02x to 0x%02x\n",A,readByte(PC));
            writeByte(readByte(PC),X);
            PC++;
            cycles=3;
            break;
        }
        case 0x8c://STY Absolute
        {

            Byte low = readByte(PC);
            PC++;
            Byte high = readByte(PC);
            PC++;
            uint16_t address = merge(high,low);
            writeByte(address,Y);
            cycles=4;
            break;
        }
        case 0x8d://STA Absolute
        {

            Byte low = readByte(PC);
            PC++;
            Byte high = readByte(PC);
            PC++;
            uint16_t address = merge(high,low);
            writeByte(address,A);
            cycles=4;
            break;
        }
        case 0x8e://STX Absolute
        {

            Byte low = readByte(PC);
            PC++;
            Byte high = readByte(PC);
            PC++;
            uint16_t address = merge(high,low);
            writeByte(address,X);
            cycles=4;
            break;
        }
        
        case 0x90:{//BCC
            Byte temp = readByte(PC);
            PC++;
            if(!SP.flag_Carry){
                int signedVal = temp;
                if(signedVal>127){
                    signedVal-=256;
                }
                int tempPC=PC+signedVal;
                if((tempPC&0xf000)!=(PC&0xf000)){
                    cycles=4;
                } else{
                    cycles=3;
                }
                PC=tempPC;
            } else{
                cycles=2;
            }
            break;
        }
        case 0xA0://LDY Immediate
            Y=readByte(PC);
            SP.flag_Zero = Y==0;
            SP.flag_Negative=Y>127;

            PC++;
            break;
        case 0xA2://LDX Immediate
            X=readByte(PC);
            SP.flag_Zero = X==0;
            SP.flag_Negative=Y>127;

            PC++;
            break;
        case 0xA5://LDA Zero Page
            A = readByte(readByte(PC));
            SP.flag_Zero = A==0;
            SP.flag_Negative=A>127;

            PC++;
            cycles=3;
            break;
        
        case 0XA9: //LDA Immediate
            A=readByte(PC);
            SP.flag_Zero = A==0;
            SP.flag_Negative=A>127;
            PC++;
            break;
        case 0xAD://LDA Absolute
        {

            Byte low = readByte(PC);
            PC++;
            Byte high = readByte(PC);
            PC++;
            A = readByte(merge(high,low));
            SP.flag_Zero = A==0;
            SP.flag_Negative=A>127;


            cycles=4;
            break;
        }
        
        case 0xB0:{//BCS
            Byte temp = readByte(PC);
            PC++;
            if(SP.flag_Carry){
                int signedVal = temp;
                if(signedVal>127){
                    signedVal-=256;
                }
                int tempPC=PC+signedVal;
                if((tempPC&0xf000)!=(PC&0xf000)){
                    cycles=4;
                } else{
                    cycles=3;
                }
                PC=tempPC;
            } else{
                cycles=2;
            }
            break;
            
        }
        case 0xD0:{//BNE
            Byte temp = readByte(PC);
            PC++;
            if(!SP.flag_Zero){
                int signedVal = temp;
                if(signedVal>127){
                    signedVal-=256;
                }
                int tempPC=PC+signedVal;
                if((tempPC&0xf000)!=(PC&0xf000)){
                    cycles=4;
                } else{
                    cycles=3;
                }
                PC=tempPC;
            } else{
                cycles=2;
            }
            break;
        }
        
        case 0xF0:{//BEQ
            Byte temp = readByte(PC);
            PC++;
            if(SP.flag_Zero){
                int signedVal = temp;
                if(signedVal>127){
                    signedVal-=256;
                }
                int tempPC=PC+signedVal;
                if((tempPC&0xf000)!=(PC&0xf000)){
                    cycles=4;
                } else{
                    cycles=3;
                }
                PC=tempPC;
            } else{
                cycles=2;
            }
            break;
        }
        default:
            printf("unknown opcode 0x%02x, crashing\n",opCode);
            exit(1);
    }
}
void Run(){
    while(cpuRunning){
        RunCPU();
    }
}
int main(int argc, char **argv){
    if(argc<2){
        fprintf(stdout,"input nes rom pls\n");
        exit(0);
    }
    filePath = argv[1];
    cpuRunning=true;
    Reset();
    printf("pc is 0x%04x \n",PC);
    Run();
    printf("%02x,%02x,%02x\n",A,X,Y);
    printf("%02x,%02x,%02x,%02x,\n",ram[0x00],ram[1],ram[2],ram[0x0550]);
    //rom exists
    //free(rom);
}
