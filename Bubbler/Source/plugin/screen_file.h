/* Write embedded OS-screen artwork without registering an unused opus_art
 * object. The OS-screen host is the sole owner of the parsed screen.
 * Only from opl_init, only self->folder/screen.svg, never audio/display code.
 * GPL-3.0-or-later. */
static int native_screen_file(struct opl_self *self,const char *path,const char *svg,unsigned len)
{
    if(!self || !path || !svg || !len || len>128u*1024u)return 0;
    /* Check every byte, not just length: replacing art must refresh its file. */
    unsigned st[13];
    if(fw_yaffs_stat(path,st)==0 && (st[2]&0x8000u) && st[7]==len) {
        int fd=fw_yaffs_open(path,0,0);
        if(fd>=0) {
            unsigned at=0;int same=1;char buf[128] __attribute__((aligned(32)));
            while(at<len && same) {
                unsigned n=len-at;if(n>sizeof buf)n=sizeof buf;
                int got=fw_yaffs_read(fd,buf,n);
                if(got<=0 || (unsigned)got>n){same=0;break;}
                for(int j=0;j<got;j++)if(buf[j]!=svg[at+(unsigned)j])same=0;
                at+=(unsigned)got;
            }
            int closed=fw_yaffs_close(fd);
            if(same && at==len && closed==0)return 1;
        }
    }
    int room=opus_flash_headroom();
    if(room<0 || (unsigned)room<len)return 0;
    /* Parent may already exist; open reports any actual creation failure. */
    fw_yaffs_mkdir(self->folder,0x4180);
    int fd=fw_yaffs_open(path,2|0x40|0x200,0x180);
    if(fd<0)return 0;
    unsigned at=0;int ok=1;
    while(at<len) {
        int n=fw_yaffs_write(fd,svg+at,len-at);
        if(n<=0 || (unsigned)n>len-at){ok=0;break;}
        at+=(unsigned)n;
    }
    if(fw_yaffs_close(fd)!=0)ok=0;
    return ok && at==len;
}
