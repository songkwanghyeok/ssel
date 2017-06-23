#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xf1334786, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x6bc3fbc0, __VMLINUX_SYMBOL_STR(__unregister_chrdev) },
	{ 0x1736c65b, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xc1d8cfaf, __VMLINUX_SYMBOL_STR(__fdget) },
	{ 0x481fe3da, __VMLINUX_SYMBOL_STR(__register_chrdev) },
	{ 0xc3f604ef, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0xc1a33dd, __VMLINUX_SYMBOL_STR(kthread_create_on_node) },
	{ 0xffcd844d, __VMLINUX_SYMBOL_STR(vfs_read) },
	{ 0x42e5c138, __VMLINUX_SYMBOL_STR(__mutex_init) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x7a74f830, __VMLINUX_SYMBOL_STR(kthread_stop) },
	{ 0xb2b06da4, __VMLINUX_SYMBOL_STR(mutex_lock) },
	{ 0xb2cde2a, __VMLINUX_SYMBOL_STR(fput) },
	{        0, __VMLINUX_SYMBOL_STR(do_sync_read) },
	{ 0x42c14039, __VMLINUX_SYMBOL_STR(wake_up_process) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
	{ 0x4b91aad3, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0xcf21d241, __VMLINUX_SYMBOL_STR(__wake_up) },
	{ 0xb3f7646e, __VMLINUX_SYMBOL_STR(kthread_should_stop) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x590a2a95, __VMLINUX_SYMBOL_STR(do_sync_write) },
	{ 0x5c1fb61b, __VMLINUX_SYMBOL_STR(__sb_end_write) },
	{ 0xefb6b186, __VMLINUX_SYMBOL_STR(interruptible_sleep_on_timeout) },
	{ 0x800687fb, __VMLINUX_SYMBOL_STR(__sb_start_write) },
	{ 0x4e5c2cc, __VMLINUX_SYMBOL_STR(vfs_write) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "97BBE34DACB72C843E5AB3B");
