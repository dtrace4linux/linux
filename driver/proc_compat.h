
/**
 * proc compat with kernel older 3.10
 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
inline struct proc_dir_entry* proc_create_data(
	const char *name, umode_t mode, struct proc_dir_entry *parent,
	const struct file_operations *proc_fops, void *data)
{
	struct proc_dir_entry *ent = create_proc_entry(name, mode, parent);
	if (ent != NULL) {
		ent->proc_fops = proc_fops;
		ent->data = data;
	}
	return ent;
}

static inline struct proc_dir_entry *proc_create(
	const char *name, umode_t mode, struct proc_dir_entry *parent,
	const struct file_operations *proc_fops)
{
	return proc_create_data(name, mode, parent, proc_fops, NULL);
}

#endif
