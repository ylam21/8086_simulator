void print_inst_profile_with_explain(Arena *arena, s32 fd, u8 clocks, u64 total_clocks, String8 formula)
{
	String8 profile = str8_fmt(arena, STR8_LIT("; +%-2d = %-3d %s"), clocks, total_clocks, formula);
	s32 written = write(fd, profile.str, profile.size);
	if (written == -1) return;
}

void print_inst_profile(Arena *arena, s32 fd, u8 clocks, u64 total_clocks)
{
	String8 profile = str8_fmt(arena, STR8_LIT("; +%-2d = %d"), clocks, total_clocks);
	s32 written = write(fd, profile.str, profile.size);
	if (written == -1) return; // TODO: print some error message to the stderr
}


void run_8086_profiler(Arena *arena, u8 *buffer, u64 read_bytes, s32 fd, u8 StartFlag)
{
	String8 header = str8_fmt(arena, STR8_LIT("%55s"), STR8_LIT("; Clock Counter (Profiler)\n"));
	s32 written = write(fd, header.str, header.size);
	if (written == -1) return;
	
    t_ctx ctx = 
    {
		.b = buffer,
        .ip = 0,
        .seg_prefix = 0xFF,
    };
	
	prof_ctx prof_ctx = {.arena = arena};
	u8 opcode = 0;
	u8 clocks = 0;
	u64 total_clocks = 0;
	
	if (StartFlag & StartFlagExplainClocks)
	{
		while (ctx.ip < read_bytes)
		{
			u64 scratch_start = arena->pos;
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
				prof_ctx.formula = (String8){0};
				prof_ctx.inst = inst;
				clocks = profilerFuncPtrTable[opcode](&prof_ctx);
			}
		
			total_clocks += clocks;
		
			print_instruction(arena, fd, inst);
			print_inst_profile_with_explain(arena, fd, clocks, total_clocks, prof_ctx.formula);
			s32 written = write(fd, "\n", 1);
			if (written == -1) return;
		
			ctx.ip += inst.size;
			arena->pos = scratch_start;
		}
	}
	else
	{
		while (ctx.ip < read_bytes)
		{
			u64 scratch_start = arena->pos;
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
				prof_ctx.inst = inst;
				clocks = profilerFuncPtrTable[opcode](&prof_ctx);
			}
		
			total_clocks += clocks;
		
			print_instruction(arena, fd, inst);
			print_inst_profile(arena, fd, clocks, total_clocks);
			s32 written = write(fd, "\n", 1);
			if (written == -1) return;
		
			ctx.ip += inst.size;
			arena->pos = scratch_start;
		}
	}
}
