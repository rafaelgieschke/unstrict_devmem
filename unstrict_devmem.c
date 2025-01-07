#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/module.h>

static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs) {
  regs_set_return_value(regs, 1);
  return 0;
}

static struct kretprobe my_kretprobe = {
    .kp = {.symbol_name = "devmem_is_allowed"},
    .handler = ret_handler,
    .maxactive = 20,
};

static int __init init(void) {
  if (register_kretprobe(&my_kretprobe) < 0)
    return -EINVAL;
  return 0;
}
module_init(init);

static void __exit cleanup(void) { unregister_kretprobe(&my_kretprobe); }
module_exit(cleanup);

MODULE_LICENSE("GPL");
