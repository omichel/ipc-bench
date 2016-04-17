#include <sys/shm.h>

#include "../common.h"

void write_data(char* shared_memory, int bytes) {
	while (*shared_memory != '1') usleep(1);
	memset(shared_memory, '*', bytes);
	*shared_memory = '2';
}

int main(int argc, const char* argv[]) {
	// The identifier for the shared memory segment
	int segment_id;

	// The *actual* shared memory, that this and other
	// processes can read and write to as if it were
	// any other plain old memory
	char* shared_memory;

	// The bytes of memory to transmit. Note that the OS
	// will round this value up to its page size (~ 4KiB).
	int bytes = get_bytes(argc, argv);

	/*
		The call that actually allocates the shared memory segment.
		Arguments:
		  1. The shared memory key. This must be unique across the OS.
			2. The number of bytes to allocate. This will be rounded up to the OS'
	       pages size for alignment purposes.
			3. The creation flags and permission bits, where:
			   - IPC_CREAT means that a new segment is to be created
				 - IPC_EXCL means that the call will fail if
           the segment-key is already taken
         - 0666 means read + write permission for user, group and world.
    When the shared memory key already exists, this call will fail. To see
    which keys are currently in use, and to remove a certain segment, you
		can use the following shell commands:
		  - Use `ipcs -m` to show shared memory segments and their IDs
		  - Use `ipcrm -m <segment_id>` to remove/deallocate a shared memory segment
	*/
	segment_id = shmget(6969, bytes, IPC_CREAT | IPC_EXCL | 0666);

	if (segment_id < 0) {
		throw("Error allocating segment!\n");
	}

	/*
		Once the shared memory segment has been created, it must be
		attached to the address space of each process that wishes to
		use it. For this, we pass:
      1. The segment ID returned by shmget
			2. A pointer at which to attach the shared memory segment. This
         address must be page-aligned. If you simply pass NULL, the OS
         will find a suitable region to attach the segment.
      3. Flags, such as:
         - SHM_RND: round the second argument (the address at which to
           attach) down to a multiple of the page size. If you don't
           pass this flag but specify a non-null address as second argument
           you must ensure page-alignment yourself.
         - SHM_RDONLY: attach for reading only (independent of access bits)
    shmat will return a pointer to the address space at which it attached the
    shared memory. Children processes created with fork() inherit this segment.
	*/
	shared_memory = (char*) shmat(segment_id, NULL, 0);

	if (shared_memory < (char*)0) {
		throw("Error attaching segment");
	}

	write_data(shared_memory, bytes);

	// Detach the shared memory from this process' address space.
	// If this is the last process using this shared memory, it is removed.
	shmdt(shared_memory);

	/*
		Deallocate manually for security. We pass:
      1. The shared memory ID returned by shmget.
			2. The IPC_RMID flag to schedule removal/deallocation
         of the shared memory.
      3. NULL to the last struct parameter, as it is not relevant
         for deletion (it is populated with certain fields for other
				 calls, notably IPC_STAT, where you would pass a struct shmid_ds*).
	*/
	shmctl(segment_id, IPC_RMID, NULL);

	return EXIT_SUCCESS;
}