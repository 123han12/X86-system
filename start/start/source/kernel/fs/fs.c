#include "fs/fs.h"
#include "common/types.h"
#include "common/cpu_instr.h"
#include "common/boot_info.h"
#include "tools/klib.h"
#include "tools/log.h"
#include "dev/console.h"
#include "fs/file.h"
#include "dev/dev.h"
#include "core/task.h"
#include "tools/list.h"
#include "cpu/irq.h"
#include <sys/file.h>
#include "dev/disk.h"

#define FS_TABLE_SIZE 10

extern fs_op_t devfs_op;

// 做一个文件系统管理表，每一个表项都是不同的文件系统
static list_t mounted_list;
static fs_t fs_table[FS_TABLE_SIZE];
static list_t free_list;

#define TEMP_FILE_ID 100
static uint8_t TEMP_ADDR[100 * 1024];
static uint8_t *temp_pos;


static int is_fd_bad(int file)
{
    if (file < 0 || file >= TASK_OFILE_NR)
    {
        return 1;
    }

    return 0;
}

static int is_path_vaild(const char *path)
{
    if (path == (const char *)0 || path[0] == '\0')
    {
        return 0;
    }
    return 1;
}

static void fs_protect(fs_t *fs)
{
    if (fs->mutex)
    {
        mutex_lock(fs->mutex);
    }
}

static void fs_unprotect(fs_t *fs)
{
    if (fs->mutex)
    {
        mutex_unlock(fs->mutex);
    }
}

// 判断指定的path路径是不是以str开头
static int path_begin_with(const char *path, const char *str)
{
    const char *s1 = path;
    const char *s2 = str;
    while (*s1 && *s2 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *s2 == '\0';
}

/// @brief 根据指定的路径打开文件，如果是设备文件，则调用dev_open函数
/// @param name
/// @param flags
/// @return
int sys_open(const char *name, int flags)
{
    if (kernel_memcmp((void *)name, (void *)"/shell.elf", 3) == 0)
    {
        int dev_id = dev_open(DEV_DISK , 0xa0 , (void*)0) ;
        dev_read(dev_id , 5000 , (uint8_t*)TEMP_ADDR , 80 ) ; // 读取elf文件到内存指定的地址中
        temp_pos = TEMP_ADDR ; 
        return TEMP_FILE_ID;
    }
    file_t *file = file_alloc();
    int fd = -1;
    if (!file)
    {
        return -1;
    }
    fd = task_alloc_fd(file);
    if (fd < 0)
    {
        goto sys_open_failed;
    }

    fs_t *fs = (fs_t *)0;
    list_node_t *node = list_first(&mounted_list);
    list_node_t *end = node;
    while (node)
    {
        fs_t *curr = list_parent_node(node, fs_t, node);
        if (path_begin_with(name, curr->mount_point))
        {
            fs = curr;
            break;
        }
        node = list_node_next(node);
        if (node == end)
            break;
    }
    if (fs)
    {
        name = path_next_child(name);
    }
    else
    {
    }

    file->mode = flags;
    file->fs = fs;
    kernel_strncpy(file->file_name, name, FILE_NAME_SIZE); // 设置文件名称

    fs_protect(fs);
    int err = fs->op->open(fs, name, file); // 调用指定文件系统的回调函数处理
    if (err < 0)
    {
        fs_unprotect(fs);
        log_printf("open %s failed..", name);
        goto sys_open_failed;
    }
    fs_unprotect(fs);
    return fd;
sys_open_failed:
    if (fd >= 0)
    {
        task_remove_fd(fd);
    }
    return -1;
}

// 将在TEMP_ADDR中的elf文件读入到指定的地址ptr 中,
int sys_read(int file, char *ptr, int len)
{
    if (TEMP_FILE_ID == file)
    {
        kernel_memcpy((void *)ptr, (void *)temp_pos, len);
        temp_pos += len;
        return len;
    }
    if(is_fd_bad(file) || !ptr || !len ) {
        return -1 ; 
    }

    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("file not opened...");
        return -1;
    }
    if(p_file->mode == O_WRONLY ) {
        log_printf("file is write only...") ; 
        return -1 ; 
    }
    fs_t* fs = p_file->fs ; 

    fs_protect(fs) ; 
    int err = fs->op->read(ptr , len , p_file) ; 
    fs_unprotect(fs) ; 

    return err ; 
}

// 向file文件中进行写，首地址为ptr 长度为len , newlib 库传过来的file 是1
int sys_write(int file, char *ptr, int len)
{
    if(is_fd_bad(file) || !ptr || !len ) {
        return -1 ; 
    }

    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("file not opened...");
        return -1;
    }
    if(p_file->mode == O_RDONLY ) {
        log_printf("file is write only...") ; 
        return -1 ; 
    }
    fs_t* fs = p_file->fs ; 

    fs_protect(fs) ; 
    int err = fs->op->write(ptr , len , p_file) ; 
    fs_unprotect(fs) ; 
    return err ; 
}
int sys_lseek(int file, int ptr, int dir)
{
    if (file == TEMP_FILE_ID)
    {
        temp_pos = (uint8_t *)(TEMP_ADDR + ptr);
        return 0;
    }


    if(is_fd_bad(file) ) {
        return -1 ; 
    }

    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("file not opened...");
        return -1;
    }

    fs_t* fs = p_file->fs ; 
    fs_protect(fs) ; 
    int err = fs->op->seek(p_file , ptr , dir ) ; 
    fs_unprotect(fs) ; 
    return err ;
}

int sys_close(int file)
{
    if (file == TEMP_FILE_ID)
    {
        return 0;
    }

    if(is_fd_bad(file) ) {
        log_printf("file error") ; 
        return 0 ; 
    }

    file_t* p_file = task_file(file) ; 
    if(!p_file  ) {
        log_printf("file not opened...");
        return -1;
    }

    ASSERT(p_file->ref > 0 ) ; 


    if(p_file->ref-- == 1 ) {
        fs_t* fs = p_file->fs ; 
        fs_protect(fs) ;  
        fs->op->close(p_file) ;
        fs_unprotect(fs) ; 

        file_free(p_file) ;  
    }
    task_remove_fd(file) ; 

    return 0;
}

int sys_isatty(int file)
{
    if(is_fd_bad(file)  ) {
        return -1 ; 
    }

    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("file not opened...");
        return -1;
    }
    return p_file->type = FILE_TTY ; 
}
int sys_fstat(int file, struct stat *st)
{
    if(is_fd_bad(file)  ) {
        return -1 ; 
    }

    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("file not opened...");
        return -1;
    }

    fs_t* fs = p_file->fs ; 

    kernel_memset(st , 0 , sizeof(struct stat))  ;


    fs_protect(fs) ; 
    int err = fs->op->stat(p_file , st ) ;
    fs_unprotect(fs) ; 
    return err ; 

}

static void mount_list_init(void)
{
    list_init(&free_list);
    for (int i = 0; i < FS_TABLE_SIZE; i++)
    {
        list_insert_first(&free_list, &fs_table[i].node);
    }
    list_init(&mounted_list);
}

static fs_op_t *get_fs_op(fs_type_t type, int major)
{
    switch (type)
    {
    case FS_DEVFS:
        return &devfs_op;
        break;
    default:
        return (fs_op_t *)0;
        break;
    }
}

static fs_t *mount(fs_type_t type, char *mount_point, int dev_major, int dev_minor)
{
    fs_t *fs = (fs_t *)0;
    log_printf("mount file system , name:%s , dev:%d\n", mount_point, dev_major);

    list_node_t *curr = list_first(&mounted_list);
    list_node_t *end = curr;

    while (curr)
    {
        fs_t *ptr = list_parent_node(curr, fs_t, node);
        if (kernel_memcmp(fs->mount_point, mount_point, FS_MOUNT_SIZE) == 0)
        {
            log_printf("fs aleardy mounted");
            goto mount_failed;
        }

        curr = list_node_next(curr);
        if (curr == end)
            break;
    }

    list_node_t *free_node = list_remove_first(&free_list);
    if (!free_node)
    {
        log_printf("no free fs , mount failed..");
        goto mount_failed;
    }
    fs = list_parent_node(free_node, fs_t, node);

    // 初始化这个fs的字段
    fs_op_t *op = get_fs_op(type, dev_major);
    if (op == (fs_op_t *)0)
    {
        log_printf("unsupported fs type!:%d", type);
        goto mount_failed;
    }

    kernel_memset(fs, 0, sizeof(fs));
    kernel_memcpy(fs->mount_point, mount_point, FS_MOUNT_SIZE);
    fs->op = op;

    if (op->mount(fs, dev_major, dev_minor) < 0)
    {
        log_printf("mount fs %s is failed..", mount_point);
        goto mount_failed;
    }

    list_insert_first(&mounted_list, &fs->node);

    return fs;
mount_failed:
    if (fs)
    {
        list_insert_last(&free_list, &fs->node);
    }
    return (fs_t *)0;
}

void fs_init(void)
{
    file_table_init();
    mount_list_init();

    // 在文件系统初始化的时候，将磁盘初始化好 
    disk_init() ; 


    fs_t *fs = mount(FS_DEVFS, "/dev", 0, 0);
    ASSERT(fs != (fs_t *)0);
}



int sys_dup(int file)
{
    if (is_fd_bad(file))
    {
        log_printf("file %d is not vaild..", file);
        return -1;
    }

    file_t *p_file = task_file(file);
    if (!p_file)
    {
        log_printf("file not opend....");
        return -1;
    }

    int fd = task_alloc_fd(p_file);

    if (fd >= 0)
    {
        file_inc_ref(p_file);
        return fd;
    }

    log_printf("no task file avaliable.");
    return -1;
}

// 提取出路径中的次设备号放入num中
int path_to_num(const char *path, int *num)
{
    // tty0
    int n = 0;
    const char *c = path;
    while (*c)
    {
        n = n * 10 + *c - '0';
        c++;
    }
    *num = n;
    return 0;
}

// 提取出path中的下一个路径
const char *path_next_child(const char *path)
{
    const char *c = path;
    while (*c && (*c == '/'))
        c++;
    while (*c && (*c != '/'))
        c++;
    while (*c && (*c == '/'))
        c++;

    return *c ? c : (const char *)0;
}