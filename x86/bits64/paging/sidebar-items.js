initSidebarItems({"constant":[["BASE_PAGE_SHIFT","Log2 of base page size (12 bits)."],["BASE_PAGE_SIZE","Size of a base page (4 KiB)"],["CACHE_LINE_SIZE","Size of a cache-line"],["HUGE_PAGE_SIZE","Size of a huge page (1 GiB)"],["LARGE_PAGE_SIZE","Size of a large page (2 MiB)"],["MAXPHYADDR","MAXPHYADDR, which is at most 52; (use CPUID for finding system value)."],["PAGE_SIZE_ENTRIES","Page tables have 512 = 4096 / 64 entries."],["PML4_SLOT_SIZE","Size of a region covered by a PML4 Entry (512 GiB)"]],"fn":[["pd_index","Given virtual address calculate corresponding entry in PD."],["pdpt_index","Given virtual address calculate corresponding entry in PDPT."],["pml4_index","Given virtual address calculate corresponding entry in PML4."],["pml5_index","Given virtual address calculate corresponding entry in PML5."],["pt_index","Given virtual address calculate corresponding entry in PT."]],"struct":[["HugePage","A type wrapping a huge page with a 1 GiB buffer."],["IOAddr","A wrapper for an IO address (IOVA / DMA Address for devices)"],["LargePage","A type wrapping a large page with a 2 MiB buffer."],["PAddr","A wrapper for a physical address."],["PDEntry","A PD Entry consists of an address and a bunch of flags."],["PDFlags","PD configuration bits description."],["PDPTEntry","A PDPT Entry consists of an address and a bunch of flags."],["PDPTFlags","PDPT configuration bit description."],["PML4Entry","A PML4 Entry consists of an address and a bunch of flags."],["PML4Flags","PML4 configuration bit description."],["PML5Entry","A PML5 Entry consists of an address and a bunch of flags."],["PML5Flags","PML5 configuration bit description."],["PTEntry","A PT Entry consists of an address and a bunch of flags."],["PTFlags","PT Entry bits description."],["Page","A type wrapping a base page with a 4 KiB buffer."],["VAddr","A wrapper for a virtual address."]],"type":[["PD","A page directory."],["PDPT","A page directory pointer table."],["PML4","A PML4 table."],["PML5","A PML5 table"],["PT","A page table."]]});