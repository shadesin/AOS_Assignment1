#include <unistd.h> // read, write, lseek, close, _exit
#include <fcntl.h> // open
#include <sys/stat.h> // stat, fstat, file permission macros
#include <sys/mman.h>  // mmap, munmap
#include <sys/types.h> // off_t

//------------UTILITY FUNCTIONS------------

// Finding length of a string
int strLength(const char* str)
{
    int len=0;
    while(str[len]!='\0')
    {
        len++;
    }
    return len;
}
// Write a string to a file descriptor
void fdWriteStr(int fd, const char* str)
{
    write(fd,str,strLength(str));
}

// Writing "Yes" or "No" to stdout depending on condition
void fdWriteYesNo(int condition)
{
    if(condition)
    {
        fdWriteStr(1,"Yes\n");
    }
    else
    {
        fdWriteStr(1,"No\n");
    }
}

// Converting a numeric string to long integer; returns -1 if invalid
long convertToLong(const char* str)
{
    long value=0;
    int i=0;

    if(str[0]=='\0')
    {
        return -1; // Empty string
    }
    while(str[i])
    {
        if(str[i]<'0' || str[i]>'9')
        {
            return -1;
        }
        value=value*10+(str[i]-'0');
        i++;
    }
    return value;
}

// Checking if buffer a[] is the reverse of buffer b[] for n bytes
int isReverse(const char* a, const char* b, long n)
{
    for(long i=0;i<n;i++)
    {
        if(a[i]!=b[n-1-i])
        {
            return 0;
        }
    }
    return 1;
}

// Printing permission checks for user/group/others on a given file or directory
void printPermissionsFor(const char* name, struct stat *st)
{
    fdWriteStr(1, "User has read permissions on ");
    fdWriteStr(1,name);
    fdWriteStr(1,": ");
    fdWriteYesNo(st->st_mode & S_IRUSR);
    fdWriteStr(1, "User has write permission on ");
    fdWriteStr(1,name);
    fdWriteStr(1,": ");
    fdWriteYesNo(st->st_mode & S_IWUSR);
    fdWriteStr(1, "User has execute permission on ");
    fdWriteStr(1,name);
    fdWriteStr(1,": ");
    fdWriteYesNo(st->st_mode & S_IXUSR);

    fdWriteStr(1, "Group has read permissions on ");
    fdWriteStr(1,name);
    fdWriteStr(1,": ");
    fdWriteYesNo(st->st_mode & S_IRGRP);
    fdWriteStr(1, "Group has write permission on ");
    fdWriteStr(1,name);
    fdWriteStr(1,": ");
    fdWriteYesNo(st->st_mode & S_IWGRP);
    fdWriteStr(1, "Group has execute permission on ");
    fdWriteStr(1,name);
    fdWriteStr(1,": ");
    fdWriteYesNo(st->st_mode & S_IXGRP);

    fdWriteStr(1, "Others have read permissions on ");
    fdWriteStr(1,name);
    fdWriteStr(1,": ");
    fdWriteYesNo(st->st_mode & S_IROTH);
    fdWriteStr(1, "Others have write permission on ");
    fdWriteStr(1,name);
    fdWriteStr(1,": ");
    fdWriteYesNo(st->st_mode & S_IWOTH);
    fdWriteStr(1, "Others have execute permission on ");
    fdWriteStr(1,name);
    fdWriteStr(1,": ");
    fdWriteYesNo(st->st_mode & S_IXOTH);
}

//--------------CONTENT CHECK FUNCTIONS---------------

// Flag 0: Block wise reversal
int checkFlag0(const char* newFile, const char* oldFile, long blockSize)
{
    int fd_new=open(newFile,O_RDONLY);
    int fd_old=open(oldFile,O_RDONLY);
    if(fd_new<0 || fd_old<0)
    {
        return 0;
    }
    char* buf_new=(char*)mmap(NULL,blockSize,PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
    char* buf_old=(char*)mmap(NULL,blockSize,PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
    if(buf_new==MAP_FAILED || buf_old==MAP_FAILED)
    {
        return 0;
    }

    int ok=1;
    ssize_t r1,r2;

    while(1)
    {
        r1=read(fd_new,buf_new,blockSize);
        r2=read(fd_old,buf_old,blockSize);

        if(r1!=r2)
        { 
            ok=0;
            break;
        }
        if(r1<=0)
        {
            break;
        }
        if(!isReverse(buf_new,buf_old,r1))
        {
            ok=0;
            break;
        }
    }

    munmap(buf_new,blockSize);
    munmap(buf_old,blockSize);
    close(fd_new);
    close(fd_old);
    return ok;
}

// Flag 1: Full file reversal
int checkFlag1(const char* newFile, const char* oldFile, long chunkSize)
{
    int fd_new=open(newFile,O_RDONLY);
    int fd_old=open(oldFile,O_RDONLY);
    if(fd_new<0 || fd_old<0)
    {
        return 0;
    }
    struct stat st;
    if(stat(newFile,&st)<0)
    {
        return 0;
    }
    long fileSize=st.st_size;
    long offset=0;
    char* buf_new=(char*)mmap(NULL,chunkSize,PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
    char* buf_old=(char*)mmap(NULL,chunkSize,PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
    int ok=1;
    while(offset<fileSize)
    {
        long sz=(chunkSize<(fileSize-offset))?chunkSize:(fileSize-offset);
        lseek(fd_old, offset, SEEK_SET);
        read(fd_old,buf_old,sz);
        lseek(fd_new,fileSize-offset-sz,SEEK_SET);
        read(fd_new,buf_new,sz);
        if(!isReverse(buf_new,buf_old,sz))
        {
            ok=0;
            break;
        }
        offset+=sz;
    }

    munmap(buf_new,chunkSize);
    munmap(buf_old,chunkSize);
    close(fd_new);
    close(fd_old);

    return ok;
}

// Flag 2: Partial range reversal
int checkFlag2(const char* newFile, const char* oldFile, long start, long end, long chunkSize)
{
    struct stat st;
    if(stat(oldFile,&st)<0)
    {
        return 0;
    }
    long fileSize=st.st_size;
    if(start>=fileSize || end >= fileSize || start >= end)
    {
        return 0;
    }
    int fd_new=open(newFile,O_RDONLY);
    int fd_old=open(oldFile,O_RDONLY);
    if(fd_new<0 || fd_old<0)
    {
        return 0;
    }
    char* buf_new=(char*)mmap(NULL,chunkSize,PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
    char* buf_old=(char*)mmap(NULL,chunkSize,PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
    int ok=1;

    // Part A: Before start-should be reversed
    long len1=start;
    long off=0;
    while(off<len1&&ok)
    {
        long sz;
        if(chunkSize<(len1-off))
        {
            sz=chunkSize;
        }
        else
        {
            sz=len1-off;
        }
        lseek(fd_new,off,SEEK_SET);read(fd_new,buf_new,sz);
        lseek(fd_old,len1-off-sz,SEEK_SET);read(fd_old,buf_old,sz);
        if(!isReverse(buf_new,buf_old,sz))
        {
            ok=0;
        }
        off+=sz;
    }

    // Part B: Middle section-unchanged
    long len2=end-start+1;
    off=0;
    while(ok && off<len2)
    {
        long sz;
        if(chunkSize<(len2-off))
        {
            sz=chunkSize;
        }
        else
        {
            sz=len2-off;
        }
        lseek(fd_new,start+off,SEEK_SET);
        read(fd_new,buf_new,sz);
        lseek(fd_old,start+off,SEEK_SET);
        read(fd_old,buf_old,sz);
        for(long i=0;i<sz;i++)
        {
            if(buf_new[i]!=buf_old[i])
            {   
                ok=0;
                break;
            }
        }
        off+=sz;
    }

    // Part C: After end-should be reversed
    long len3=fileSize-end-1;
    off=0;
    while(ok && off<len3)
    {
        long sz;
        if(chunkSize<(len3-off))
        {
            sz=chunkSize;
        }
        else
        {
            sz=len3-off;
        }
        lseek(fd_new,end+1+off,SEEK_SET);
        read(fd_new,buf_new,sz);
        lseek(fd_old,fileSize-(off+sz),SEEK_SET);
        read(fd_old,buf_old,sz);
        if(!isReverse(buf_new,buf_old,sz))
        {
            ok=0;
        }
        off+=sz;
    }

    munmap(buf_new, chunkSize);
    munmap(buf_old, chunkSize);
    close(fd_new);
    close(fd_old);

    return ok;
}

//------------------MAIN------------------

int main(int argc, char* argv[])
{
    if(argc<5)
    {
        fdWriteStr(2,"Invalid arguments\n");
        _exit(1);
    }
    const char* newFile=argv[1];
    const char* oldFile=argv[2];
    const char* dirPath=argv[3];
    int flag=(int)convertToLong(argv[4]);
    long blockSize=0,start=0,end=0;
    if(flag==0)
    {
        if(argc!=6)
        {
            _exit(1);
        }
        blockSize=convertToLong(argv[5]);
    }
    else if(flag==1)
    {
        if(argc!=5)
        {
            _exit(1);
        }
        blockSize=1024*1024;
    }
    else if(flag==2)
    {
        if (argc!=7)
        {
            _exit(1);
        }
        start=convertToLong(argv[5]);
        end=convertToLong(argv[6]);
        blockSize=1024*1024;
    }
    else _exit(1);
    struct stat st_new,st_old,st_dir;
    int new_ok=(stat(newFile,&st_new)==0);
    int old_ok=(stat(oldFile,&st_old)==0);

    // Check directory first and print as first result line
    int dir_ok=(stat(dirPath,&st_dir)==0 && S_ISDIR(st_dir.st_mode));
    fdWriteStr(1,"Directory is created: ");
    fdWriteYesNo(dir_ok);
    if(!dir_ok)
    {
        fdWriteStr(2,"ERROR: Directory missing or not a valid directory.\n");
        _exit(1);
    }

    // File content validation
    int content_ok=0;
    if(flag==0)
    {
        if(new_ok && old_ok)
        {
            content_ok=checkFlag0(newFile,oldFile,blockSize);
        }
        else
        {
            content_ok=0;
        }

    }
    else if(flag==1)
    {
        if(new_ok && old_ok)
        {
            content_ok=checkFlag1(newFile,oldFile,blockSize);
        }
        else
        {
            content_ok=0;
        }
    }
    else if(flag==2)
    {
        if(new_ok && old_ok)
        {
            content_ok=checkFlag2(newFile,oldFile,start,end,blockSize);
        }
        else
        {
            content_ok=0;
        }

    }
    fdWriteStr(1,"Whether file contents are correctly processed: ");
    fdWriteYesNo(content_ok);

    // File size check
    int size_same=(new_ok && old_ok && st_new.st_size==st_old.st_size);
    fdWriteStr(1,"Both Files Sizes are Same: ");
    fdWriteYesNo(size_same);

    // Permissions-new file
    if(new_ok)
    {
        printPermissionsFor("new file", &st_new);
    }
    else
    {
        for(int i=0;i<9;i++)
        fdWriteYesNo(0);
    }

    // Permissions-old file
    if(old_ok)
    {
        printPermissionsFor("old file",&st_old);
    }
    else
    {
        for(int i=0;i<9;i++)
        fdWriteYesNo(0);
    }

    // Permissions-directory
    printPermissionsFor("directory",&st_dir);

    return 0;
}