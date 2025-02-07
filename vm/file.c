/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"

/* project3 */
#include "userprog/process.h"
#include "threads/mmu.h"
/* project3 */


static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;
	struct file_page *file_page = &page->file;

  return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;

    /* project3 */
	if (page == NULL)
        return false;

  struct container *aux = (struct container *)page->uninit.aux;

  struct file *file = aux->file;
	off_t offset = aux->offset;
  size_t page_read_bytes = aux->page_read_bytes;
  size_t page_zero_bytes = PGSIZE - page_read_bytes;

	file_seek (file, offset);

  if (file_read (file, kva, page_read_bytes) != (int) page_read_bytes) {
      return false;
  }

  memset (kva + page_read_bytes, 0, page_zero_bytes);

  return true;
  /* project3 */
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
  /* project3 */
  if (page == NULL)
      return false;

  struct container * aux = (struct container *) page->uninit.aux;

  // check dirty page
  if(pml4_is_dirty(thread_current()->pml4, page->va)){
    file_write_at(aux->file, page->va, aux->page_read_bytes, aux->offset);
    pml4_set_dirty (thread_current()->pml4, page->va, 0);
  }

  pml4_clear_page(thread_current()->pml4, page->va);
  if (!page->frame){
    page->frame->page = NULL;
    page->frame = NULL; //p3mtp need?
  }
  // page->frame = NULL; //p3tmp need?
	return true;
  /* project3 */

}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}


/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
  /* project3 */
	struct file *mfile = file_reopen(file);

  void * ori_addr = addr;
  size_t read_bytes = length > file_length(file) ? file_length(file) : length;
  size_t zero_bytes = PGSIZE - read_bytes % PGSIZE;
  // struct page *page;
  while (read_bytes > 0 || zero_bytes > 0) {
    size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
    size_t page_zero_bytes = PGSIZE - page_read_bytes;

    struct container *container = (struct container*)malloc(sizeof(struct container));
    container->file = mfile;
    container->offset = offset;
    container->page_read_bytes = page_read_bytes;

    if (!vm_alloc_page_with_initializer (VM_FILE, addr, writable, lazy_load_segment, container)) {
      free(container);
      return NULL;
    }
    read_bytes -= page_read_bytes;
    zero_bytes -= page_zero_bytes;
    addr       += PGSIZE;
    offset     += page_read_bytes;
  }
  return ori_addr;
  /* project3 */
}

/* Do the munmap */
void
do_munmap (void *addr) {
    /* project3 */
  while (true) {
    struct page* page = spt_find_page(&thread_current()->spt, addr);
    if (page == NULL)
      break;
    // 변경사항에 대한 정보를 쓰기.
    struct container * aux = (struct container *) page->uninit.aux;

    // check dirty bit
    if(pml4_is_dirty(thread_current()->pml4, page->va)) {
      file_write_at(aux->file, addr, aux->page_read_bytes, aux->offset);
      pml4_set_dirty (thread_current()->pml4, page->va, 0);
    }

    pml4_clear_page(thread_current()->pml4, page->va);
    addr += PGSIZE;
  }
  /* project3 */
}
