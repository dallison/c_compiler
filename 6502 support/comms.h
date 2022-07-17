
// Datagram.
.set datagram_seqnum 3
.set datagram_acknum 4
.set datagram_ack 5
.set datagram_data 6      // Contain message.

// Message command codes.
.set ping_command_code 1
.set list_files_command_code 2
.set list_files_result_code 3
.set dir_entry_code 4
.set load_file_code 5
.set load_file_result_code 6
.set file_block_code 7

// Message
.set message_command 0      // Command code.
.set message_length 1       // Length of data.
.set message_data 2         // Data starts here.

// Dir entry.
.set dir_entry_length 0
.set dir_entry_type 2
.set dir_entry_name 3

.set dir_entry_dir 0
.set dir_entry_file 1

.set load_raw 0
.set load_binary 1

.set load_file_mode 0
.set load_file_filename 1

.set load_file_result_length 0
.set load_file_result_entry_addr 2
.set load_file_result_error 4

.set file_block_result_addr 0
.set file_block_result_data 2

