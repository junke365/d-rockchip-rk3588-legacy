cmd_drivers/firmware/efi/libstub/relocate.stub.o := aarch64-none-linux-gnu-strip --strip-debug -o drivers/firmware/efi/libstub/relocate.stub.o drivers/firmware/efi/libstub/relocate.o; if aarch64-none-linux-gnu-objdump -r drivers/firmware/efi/libstub/relocate.stub.o | grep R_AARCH64_ABS; then echo "drivers/firmware/efi/libstub/relocate.stub.o: absolute symbol references not allowed in the EFI stub" >&2; /bin/false; fi; aarch64-none-linux-gnu-objcopy --remove-section=.note.gnu.property --prefix-alloc-sections=.init --prefix-symbols=__efistub_ drivers/firmware/efi/libstub/relocate.o drivers/firmware/efi/libstub/relocate.stub.o
