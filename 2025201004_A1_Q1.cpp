#include <fcntl.h>       // open, O_*
#include <unistd.h>      // read, write, lseek, close, _exit
#include <sys/stat.h>    // mkdir
#include <errno.h>       // errno
#include <sys/mman.h>    // mmap, munmap
#include <sys/types.h>   // off_t

// ----------------UTILITY FUNCTIONS---------------

//Finding length of a string
int strLength(const char* str)
{
    int len=0;
    while(str[len]!='\0')
    {
        len++;
    }
    return len;
}

//String to int converter. This functions returns -1 if the string starts with a minus symbol or contains characters other than '0' to '9'
int convertToInt(const char* str)
{
    int val=0,i=0;
    if(str[0]=='-')
    {
        return -1;
    }
    while(str[i])
    {
        if(str[i]<'0' || str[i]>'9')
        {
            return -1;
        }
        val=val*10+(str[i]-'0');
        i++;
    }
    return val;
}

//Writing a string to a file descriptor
void fdWriteStr(int fd, const char* str)
{
    write(fd, str, strLength(str));
}

//Writing an integer to a file descriptor
void fdWriteInt(int fd, int num)
{
    char buffer[12];
    buffer[11]='\0';
    int i=10;
    if(num==0)
    {
        write(fd, "0", 1);
        return;
    }
    bool neg=false;
    if(num<0)
    {
        neg=true;
        num=-num;
    }
    while(num>0 && i>=0)
    {
        buffer[i--]='0'+(num%10);
        num/=10;
    }
    if(neg && i>=0)
    {
        buffer[i--]='-';
    }
    write(fd,buffer+i+1,11-i-1);
}

//Prints correct usage syntax if the input command syntax does not match
void printUsage()
{
    fdWriteStr(2,"Usage:\n");
    fdWriteStr(2,"./a.out <input_file> 0 <block_size>\n");
    fdWriteStr(2,"./a.out <input_file> 1\n");
    fdWriteStr(2,"./a.out <input_file> 2 <start_index> <end_index>\n");
}

//-----------------MAIN------------------

int main(int argc, char* argv[])
{
    if(argc<3)
    {
        printUsage();
        _exit(1);
    }

    const char* inputFile=argv[1];
    int mode=convertToInt(argv[2]);

    //Validating args per mode
    int blockSize=0,start=0,end=0;
    if(mode==0)
    {
        if(argc!=4)
        {
            fdWriteStr(2,"Flag 0 requires block size.\n");
            printUsage();
            _exit(1);
        }
        blockSize=convertToInt(argv[3]);
        if(blockSize<=0)
        {
            fdWriteStr(2,"Invalid block size.\n");
            _exit(1);
        }
        if(blockSize>8*1024*1024)
        {
            fdWriteStr(2,"Warning: block > *MB.\n");
            _exit(1);
        }
    }
    else if(mode==1)
    {
        if(argc!=3)
        {
            fdWriteStr(2,"Flag 1 takes no extra args.\n");
            printUsage();
            _exit(1);
        } 
        blockSize = 1024*1024;
    }
    else if(mode==2)
    {
        if (argc!=5)
        {
            fdWriteStr(2,"Flag 2 requires start and end indices.\n");
            printUsage();
            _exit(1);
        }
        start=convertToInt(argv[3]);
        end=convertToInt(argv[4]);
        if(start<0 || end<0 || start>=end)
        {
            fdWriteStr(2,"Invalid start/end indices.\n");
            _exit(1);
        }
        blockSize=1024*1024; //chunk size for revresed parts
    }
    else
    {
        fdWriteStr(2,"Only flags 0, 1, 2 supported.\n");
        _exit(1);
    }
    
    //Ensure Assignment1 directory
    if(mkdir("Assignment1",0700)==-1 && errno!=EEXIST)
    {
        fdWriteStr(2,"Assignment1 dirctory creation failed!\n");
        _exit(1);
    }

    //Open input
    int fd_in=open(inputFile,O_RDONLY);
    if(fd_in==-1)
    {
        fdWriteStr(2,"Failed to open input!\n");
        _exit(1);
    }

    //Build output path: Assignment1/<flag>_<input_file_name>
    const char* baseName=inputFile;
    for(int i=0;inputFile[i]!='\0';i++)
    {
        if(inputFile[i]=='/')
        {
            baseName=inputFile+i+1;
        }
    }
    char outputPath[512];
    int pos=0;
    const char* folder="Assignment1/";
    for(int i=0;folder[i];i++)
    {
        outputPath[pos]=folder[i];
        pos++;
    }
    outputPath[pos++]='0'+mode;
    outputPath[pos++]='_';
    for(int i=0;baseName[i];i++)
    {
        outputPath[pos]=baseName[i];
        pos++;
    }
    outputPath[pos]='\0';

    //Open output
    int fd_out=open(outputPath,O_CREAT|O_WRONLY|O_TRUNC,0000);
    if(fd_out==-1)
    {
        fdWriteStr(2,"Failed to open output\n");
        close(fd_in);
        _exit(1);
    }

    //Get file size
    off_t fileSize=lseek(fd_in,0,SEEK_END);
    if(fileSize==(off_t)-1)
    {
        fdWriteStr(2,"Failed to get file size!\n");
        close(fd_in);
        close(fd_out);
        _exit(1);
    }

    if(mode==2 && (start>=fileSize || end>=fileSize || start>=end))
    {
        fdWriteStr(2,"Start/end indices out of range!\n");
        close(fd_in);
        close(fd_out);
        _exit(1);
    }

    //Map buffer for chunk operations
    char* buffer=(char*)mmap(NULL,blockSize,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(buffer==MAP_FAILED)
    {
        fdWriteStr(2,"Buffer allocation failed!\n");
        close(fd_in);
        close(fd_out);
        _exit(1);
    }

    //Reset input pointer for sequential reads
    lseek(fd_in,0,SEEK_SET);

    //-----------------------FLAG IMPLEMENTATIONS-------------------------
    if(mode==0) //Block-wise reversal
    {
        off_t totalBlocks=(fileSize+blockSize-1)/blockSize,doneBlocks=0;

        while(doneBlocks<totalBlocks)
        {
            ssize_t sz=blockSize;
            off_t left=fileSize-doneBlocks*blockSize;
            if(left<blockSize)
            {
                sz=left;
            }

            ssize_t rbytes=0;
            while(rbytes<sz)
            {
                ssize_t r=read(fd_in,buffer+rbytes,sz-rbytes);
                if(r<=0)
                {
                    break;
                }
                rbytes+=r;
            }
            if(rbytes==0)
            {
                break;
            }

            //Reverse this block
            for(ssize_t i=0;i<rbytes/2;i++)
            {
                char t=buffer[i];
                buffer[i]=buffer[rbytes-i-1];
                buffer[rbytes-i-1]=t;
            }

            //Write out
            ssize_t wbytes=0;
            while(wbytes<rbytes)
            {
                ssize_t w=write(fd_out,buffer+wbytes,rbytes=wbytes);
                if(w<=0)
                {
                    break;
                }
                wbytes+=w;
            }

            doneBlocks++;

            //Progress
            write(1,"\rProgress: ",11);
            fdWriteInt(1,(int)((doneBlocks*100)/totalBlocks));
            write(1,"%",1);
        }
    }
    else if(mode==1) //Full file reversal
    {
        off_t processed=0,remaining=fileSize;

        while(remaining>0)
        {
            ssize_t sz;
            if(remaining>=blockSize)
            {
                 sz=blockSize;
            }
            else
            {
                sz=remaining;
            }
        
            if(lseek(fd_in,remaining-sz,SEEK_SET)==(off_t)-1)
            {
                break;
            }
            ssize_t rbytes=0;
            while(rbytes<sz)
            {
                ssize_t r=read(fd_in,buffer+rbytes,sz-rbytes);
                if(r<=0)
                {
                    break;
                }
                rbytes+=r;
            }
            for(ssize_t i=0;i<rbytes/2;i++)
            {
                char t=buffer[i];
                buffer[i]=buffer[rbytes-i-1];
                buffer[rbytes-i-1]=t;
            }
            ssize_t wbytes=0;
            while(wbytes<rbytes)
            {
                ssize_t w=write(fd_out,buffer+wbytes,rbytes-wbytes);
                if(w<=0)
                {
                    break;
                }
                wbytes+=w;
            }
            processed+=rbytes;
            remaining-=rbytes;

            //Progress
            write(1,"\rProgress: ",11);
            fdWriteInt(1,(int)((processed*100)/fileSize));
            write(1,"%",1);
        }
    }
    else if(mode==2) //Partial range reversal
    {
        //Part A: Reverse [0...start_index-1]
        off_t leftA=start; //count number of bytes to reverse
        off_t frontPos=0;
        off_t backPos=start-1;
        while(frontPos<backPos)
        {
            ssize_t chunk;
            if(blockSize<(backPos-frontPos+1)/2)
            {
                chunk=blockSize;
            }
            else
            {
                chunk=(backPos-frontPos+1)/2;
            }

            //Read front
            lseek(fd_in,frontPos,SEEK_SET);
            read(fd_in,buffer,chunk);
            //Read back
            lseek(fd_in,backPos-chunk+1,SEEK_SET);
            char* buffer2=(char*)mmap(NULL,chunk,PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
            read(fd_in,buffer2,chunk);

            //Swap
            for(int i=0;i<chunk;i++)
            {
                char t=buffer[i];
                buffer[i]=buffer2[chunk-i-1];
                buffer2[chunk-i-1]=t;
            }
            //Write swapped portion
            lseek(fd_out,frontPos,SEEK_SET);
            write(fd_out,buffer,chunk);
            lseek(fd_out,backPos-chunk+1,SEEK_SET);
            write(fd_out,buffer2,chunk);
            munmap(buffer2,chunk);
            frontPos+=chunk;
            backPos-=chunk;
        }

        //Part B: Copy [start_index....end_index]
        lseek(fd_in,start,SEEK_SET);
        lseek(fd_out,start,SEEK_SET);
        off_t middleSize=end-start+1;
        while(middleSize>0)
        {
            ssize_t sz;
            if(middleSize>=blockSize)
            {
                sz=blockSize;
            }
            else
            {
                sz=middleSize;
            }
            ssize_t r=read(fd_in,buffer,sz);
            if(r<=0)
            {
                break;
            }
            write(fd_out,buffer,r);
            middleSize-=r;
        }

        //Part C: Revrese [end_index+1......EOF]
        off_t front=end+1;
        off_t back=fileSize-1;
        while(front<back)
        {
            ssize_t chunk;
            if(blockSize<(back-front+1)/2)
            {
                chunk=blockSize;
            }
            else
            {
                chunk=(back-front+1)/2;
            }
            lseek(fd_in,front,SEEK_SET);
            read(fd_in,buffer,chunk);
            lseek(fd_in,back-chunk+1,SEEK_SET);
            char* buffer2=(char*)mmap(NULL,chunk,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
            read(fd_in,buffer2,chunk);

            for(int i=0;i<chunk;i++)
            {
                char t=buffer[i];
                buffer[i]=buffer2[chunk-i-1];
                buffer2[chunk-i-1]=t;
            }
            lseek(fd_out,front,SEEK_SET);
            write(fd_out,buffer,chunk);
            lseek(fd_out,back-chunk+1,SEEK_SET);
            write(fd_out,buffer2,chunk);
            munmap(buffer2,chunk);
            front+=chunk;
            back-=chunk;
        }
    }
    write(1,"\n",1);
    munmap(buffer,blockSize);
    close(fd_in);
    close(fd_out);
    return 0;
}