void print_inst_profile(Arena *arena, s32 fd, u8 clocks, u64 total_clocks)
{
	String8 profile = str8_fmt(arena, STR8_LIT("; +%d = %d"), clocks, total_clocks);
	s32 written = write(fd, profile.str, profile.size);
	if (written == -1) return;
}


void run_8086_profiler(Arena *arena, u8 *buffer, u64 read_bytes, s32 fd, u32 StartFlags)
{
	(void)StartFlags;
	u8 opcode = 0;
	

	String8 header = str8_fmt(arena, STR8_LIT("%55s"), STR8_LIT("; Clock Counter (Profiler)\n"));
	s32 written = write(fd, header.str, header.size);
	if (written == -1) return;

    t_ctx ctx = 
    {
		.b = buffer,
        .ip = 0,
        .seg_prefix = 0xFF,
    };
	
	u64 total_clocks = 0;
	u8 clocks = 0;
    while (ctx.ip < read_bytes)
    {
        ctx.b = &buffer[ctx.ip];
        opcode = ctx.b[0];
        func_ptr handler = opcode_table[opcode];
        Instruction inst = handler(&ctx);
		
		profilerFuncPtr profile = profilerFuncPtrTable[opcode];
		if (profile == NULL)
		{
			clocks = 0;
			fprintf(stderr, "Error: Profiler does not support the opcode 0x%02x\n", opcode);
		}
		else
		{
			clocks = profilerFuncPtrTable[opcode](inst);
		}

		total_clocks += clocks;

		print_instruction(arena, fd, inst);
		print_inst_profile(arena, fd, clocks, total_clocks);
		s32 written = write(fd, "\n", 1);
		if (written == -1) return;

        ctx.ip += inst.size;
        arena_reset(arena);
    }
}