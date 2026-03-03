const u8 flag_map[9] = { 
    POS_OF, POS_DF, POS_IF, POS_TF, POS_SF, POS_ZF, POS_AF, POS_PF, POS_CF 
};

void mod_ZF(u16 *reg, u32 res)
{
    if (res == 0)
    {
        *reg |= (1 << POS_ZF);
    }
    else
    {
        *reg &= ~(1 << POS_ZF);
    }
}

void mod_PF(u16 *reg, u32 res)
{
    u8 cnt = __builtin_popcount(res & 0xFF);

    if (!(cnt & 1))
    {
        *reg |= (1 << POS_PF);
    }
    else
    {
        *reg &= ~(1 << POS_PF);
    }
}

void mod_SF(u16 *reg, u32 res, u8 mask_dest)
{
    u8 width_bit;
    if (mask_dest == MASK_WIDE || mask_dest == MASK_HIGH)
    {
        width_bit = 15;
    }
    else
    {
        width_bit = 7;
    }

    if ((res >> width_bit) & 1)
    {
        *reg |= (1 << POS_SF);
    }
    else
    {
        *reg &= ~(1 << POS_SF);
    }
}

String8 create_state_of_flag_reg(Arena *arena, u16 reg)
{ 
    String8 flags = STR8_LIT("ODITSZAPC");

    u8 *str = arena_push(arena, flags.size);
    if (!str) return (String8){0};

    memcpy(str, flags.str, flags.size);

    u8 pos = 0;
    while (pos < flags.size)
    {
        u8 bit_pos = flag_map[pos];
        u8 res = (reg >> bit_pos) & 1;
        if (res == 0)
        {
            str[pos] = CHAR_SPACE;
        }
        pos += 1;
    }
 
    return (String8){ .size = flags.size, .str = str};
}


String8 state_of_flags(Arena *arena, u16 reg_old, u16 reg_new)
{
    String8 state_old = create_state_of_flag_reg(arena, reg_old);
    String8 state_new = create_state_of_flag_reg(arena, reg_new);
    return str8_fmt(arena, STR8_LIT(" [%s]->[%s]"), state_old, state_new);
}

String8 regs_names[14] =
{
    STR8_LIT("ax"), // General-purpose registers
    STR8_LIT("cx"), // 'ax' ... 'di'
    STR8_LIT("dx"),
    STR8_LIT("bx"),
    STR8_LIT("sp"),
    STR8_LIT("bp"),
    STR8_LIT("si"),
    STR8_LIT("di"),
    STR8_LIT("es"), // Segment registers
    STR8_LIT("cs"), // 'es' ... 'ds'
    STR8_LIT("ss"),
    STR8_LIT("ds"),
    STR8_LIT("ip"), // Instruction pointer register
    STR8_LIT("flags"),   // flags register
};

void print_final_regs(Arena *arena, s32 fd, u16 *regs)
{
    String8 header = STR8_LIT("\nFinal Registers:\n");
    s32 written = write(fd, header.str, header.size);
    if (written == -1) return;

    u8 idx_order[13] = {0, 3, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    u8 i = 0;
    while (i < 13)
    {
        u8 pos = idx_order[i];
        u16 res = regs[pos];
        if (res)
        {
            String8 line = str8_fmt(arena, STR8_LIT("%10s: 0x%04x (%d)\n"), regs_names[pos], regs[pos], regs[pos]);
            written = write(fd, line.str, line.size);
            if (written == -1) return;
        }
        i += 1;
    }
    String8 flags = create_state_of_flag_reg(arena, regs[FLAGS_IDX]);
    String8 flag_field;
    if (regs[FLAGS_IDX])
    {
        flag_field = str8_fmt(arena, STR8_LIT("%10s: [%s]"), regs_names[FLAGS_IDX], flags);
    }
    else
    {
        flag_field = (String8){0};
    }
    written = write(fd, flag_field.str, flag_field.size);
    if (written == -1) return;
}

void modifyDest(u16 *destPtr, u16 val, u8 mask, OperandType dType)
{
    if (dType == OP_MEMORY || dType == OP_MEMORY_DIR)
    {
        if (mask == MASK_WIDE)
        {
            *destPtr = val;
        }
        else if (mask == MASK_HIGH)
        {
            *(u8*)destPtr = (u8)val;
        }
        else
        {
            fprintf(stderr, "Error: mask flag has garbage value\n");
        }
    }
    else
    {

        if (mask == MASK_WIDE)
        {
            *destPtr = val;
        }
        else if (mask == MASK_LOW)
        {
            *destPtr = (*destPtr & 0xFF00) | val;
        }
        else if (mask == MASK_HIGH)
        {
            *destPtr = (*destPtr & 0x00FF) | (val << 8);
        }
        else
        {
            fprintf(stderr, "Error: mask flag has garbage value\n");
        }
    }
}

u8 decode_mask_from_reg(Operand reg, u8 W)
{
    if (W == 0)
    {
        if (reg.reg_idx < 4)
        {
            return MASK_LOW;
        }
        else
        {
            return MASK_HIGH;
        }
    }
    else
    {
        return MASK_WIDE;
    }
}

u16 masked_u16(u16 value, u8 mask)
{
    if (mask == MASK_WIDE)
    {
        return value;
    }
    else if (mask == MASK_LOW)
    {
        return (value & 0x00FF);
    }
    else if (mask == MASK_HIGH)
    {
        return (value & 0xFF00) >> 8;
    }
    else
    {
        fprintf(stderr, "Error: mask flag has garbage value\n");
        return 0;
    }
}

u8 decode_final_reg_idx_from_sreg(Operand sreg)
{
    return sreg.reg_idx + 8;
}

u8 decode_final_reg_idx_from_reg(Operand reg, u8 W)
{
    if (W == 0)
    {
        return (reg.reg_idx % 4);
    }
    else
    {
        return reg.reg_idx;
    }
}

void modify_flag_reg(u16 *reg, u32 res, u8 mask_dest)
{
    mod_ZF(reg, res);
    mod_PF(reg, res);
    mod_SF(reg, res, mask_dest);
}

calcMemoryAddressFunc calc_mem_table[8] =
{
    [0] = calc_bx_plus_si,
    [1] = calc_bx_plus_di,
    [2] = calc_bp_plus_si,
    [3] = calc_bp_plus_di,
    [4] = calc_si,
    [5] = calc_di,
    [6] = calc_bp,
    [7] = calc_bx,
};

u16 calc_bx_plus_si(u16 *regs) {return regs[BX_IDX] + regs[SI_IDX];};
u16 calc_bx_plus_di(u16 *regs) {return regs[BX_IDX] + regs[DI_IDX];};
u16 calc_bp_plus_si(u16 *regs) {return regs[BP_IDX] + regs[SI_IDX];};
u16 calc_bp_plus_di(u16 *regs) {return regs[BP_IDX] + regs[DI_IDX];};
u16 calc_si(u16 *regs) {return regs[SI_IDX];};
u16 calc_di(u16 *regs) {return regs[DI_IDX];};
u16 calc_bp(u16 *regs) {return regs[BP_IDX];};
u16 calc_bx(u16 *regs) {return regs[BX_IDX];};

u16 calc_memory_address(Operand op, Cpu *cpu)
{
    u16 base_address = calc_mem_table[op.mem_base_reg](cpu->regs);
    return base_address + op.mem_disp;
}

void execute_instruction(Arena *arena, s32 fd, Cpu *cpu, Instruction inst, u16 *regsStateOld, t_ctx *ctx)
{
    OperandType sType = inst.src.type;
    OperandType dType = inst.dest.type;

    u8 dest_reg_idx = 0;
    u8 mask_dest = 0;
    u16 *destPtr = 0;
    if (dType == OP_REGISTER || dType == OP_REGISTER_CL || dType == OP_REGISTER_DX)
    {
        dest_reg_idx = decode_final_reg_idx_from_reg(inst.dest, inst.w_bit);
        destPtr = &cpu->regs[dest_reg_idx];
        mask_dest = decode_mask_from_reg(inst.dest, inst.w_bit);
    }
    else if (dType == OP_MEMORY)
    {
        u16 address = calc_memory_address(inst.dest, cpu);
        mask_dest = inst.w_bit == 1 ? MASK_WIDE : MASK_HIGH;
        destPtr = (u16*)&cpu->Memory[address];
    }
    else if (dType == OP_MEMORY_DIR)
    {
        destPtr = (u16*)&cpu->Memory[inst.dest.mem_disp];
        mask_dest = inst.w_bit == 1 ? MASK_WIDE : MASK_HIGH;
    }
    else if (dType == OP_SREG)
    {
        dest_reg_idx = decode_final_reg_idx_from_sreg(inst.dest); 
        destPtr = &cpu->regs[dest_reg_idx];
        mask_dest = MASK_WIDE;
    }

    u16* ipRegPtr = &cpu->regs[IP_IDX];
    u16* cxRegPtr = &cpu->regs[CX_IDX];
    u16* flagRegPtr = &cpu->regs[FLAGS_IDX];
    u8 src_reg_idx = 0;
    u8 mask_src = 0;
    u16 src_val = 0;
    if (sType == OP_REGISTER || sType == OP_REGISTER_CL || sType == OP_REGISTER_DX)
    {
        src_reg_idx = decode_final_reg_idx_from_reg(inst.src, inst.w_bit);
        mask_src = decode_mask_from_reg(inst.src, inst.w_bit);
        src_val = masked_u16(cpu->regs[src_reg_idx], mask_src); // val is the masked 'src'
    }
    else if (sType == OP_SREG)
    {
        src_reg_idx = decode_final_reg_idx_from_sreg(inst.src);
        mask_src = MASK_WIDE;
        src_val = masked_u16(cpu->regs[src_reg_idx], mask_src);
    }
    else if (sType == OP_IMMEDIATE)
    {
        src_val = inst.src.immediate_val;
    }
    else if (sType == OP_MEMORY)
    {
        u16 address = calc_memory_address(inst.src, cpu);
        mask_src = inst.w_bit == 1 ? MASK_WIDE : MASK_HIGH;
        src_val = masked_u16(cpu->Memory[address], mask_src);
    }
    else if (sType == OP_MEMORY_DIR)
    {
        mask_src = inst.w_bit == 1 ? MASK_WIDE : MASK_HIGH;
        src_val = masked_u16(cpu->Memory[inst.src.mem_disp], mask_src);
    }
    
    u8 dont_print_flags = false;
    u8 dont_print_regs = false;
    if (!str8ncmp(inst.mnemonic, INST_MOV, INST_MOV.size))
    {   
        modifyDest(destPtr, src_val, mask_dest, dType);
        dont_print_flags = true;
    }
    else if (!str8ncmp(inst.mnemonic, INST_ADD, INST_ADD.size))
    {
        u32 res = (u32)masked_u16(*destPtr, mask_dest) + (u32)src_val;
        modify_flag_reg(flagRegPtr, res, mask_dest);
        modifyDest(destPtr, (u16)res, mask_dest, dType);
    }
    else if (!str8ncmp(inst.mnemonic, INST_SUB, INST_SUB.size))
    {
        u32 res = (u32)masked_u16(*destPtr, mask_dest) - (u32)src_val;
        modify_flag_reg(flagRegPtr, res, mask_dest);
        modifyDest(destPtr, (u16)res, mask_dest, dType);
    }
    else if (!str8ncmp(inst.mnemonic, INST_CMP, INST_CMP.size))
    {
        u32 res = (u32)masked_u16(*destPtr, mask_dest) - (u32)src_val;
        modify_flag_reg(flagRegPtr, res, mask_dest);
        dont_print_regs = true;
    }
    else if (!str8ncmp(inst.mnemonic, INST_JNZ, INST_JNZ.size))
    {
        if (!((*flagRegPtr >> POS_ZF) & 1))
        {
            ctx->ip = inst.dest.immediate_val;
            modifyDest(ipRegPtr, ctx->ip, MASK_WIDE, dType);
        }
        dont_print_flags = true;
        dont_print_regs = true;
    }
    else if (!str8ncmp(inst.mnemonic, INST_LOOP, INST_LOOP.size))
    {
        dType = OP_REGISTER;
        dest_reg_idx = CX_IDX;
        u16 cx_dec = *cxRegPtr - 1;
        modifyDest(cxRegPtr, cx_dec, MASK_WIDE, OP_REGISTER);
        mod_ZF(flagRegPtr, cx_dec);
        if (!((*flagRegPtr >> POS_ZF) & 1))
        {
            ctx->ip = inst.dest.immediate_val;
            modifyDest(ipRegPtr, ctx->ip, MASK_WIDE, dType);
        }
    }
    
    u16 *regsStateNew = cpu->regs;
    String8 res, regs, flags;
    String8 ip = str8_fmt(arena, STR8_LIT(" ip: 0x%04x->0x%04x"), regsStateOld[IP_IDX], regsStateNew[IP_IDX]);
    if (dont_print_flags)
    {
        flags = (String8){0};
    }
    else
    {
        flags = state_of_flags(arena, regsStateOld[FLAGS_IDX], regsStateNew[FLAGS_IDX]);
    }
    if (dont_print_regs || (dType != OP_REGISTER && dType != OP_REGISTER_CL && dType != OP_REGISTER_DX))
    {
        regs = str8_fmt(arena, STR8_LIT(" %18c"), CHAR_SPACE);
    }
    else
    {
        regs = str8_fmt(arena, STR8_LIT(" %s: 0x%04x->0x%04x"), regs_names[dest_reg_idx], regsStateOld[dest_reg_idx], regsStateNew[dest_reg_idx]);
    }

    res = str8_fmt(arena, STR8_LIT(" ;%s%s%s"), regs, ip, flags);

    s32 written = write(fd, res.str, res.size);
    if (written == -1) return;
}

void write_memory(u8 *memory, u64 size)
{
    const char *filename = "sim86_memory.data";
    s32 fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd == -1)
    {
        perror(filename);
        return;
    }
    s32 written = write(fd, memory, size);
    if (written == -1) return;
    fprintf(stdout, "Written dump data to: \"%s\"\n", filename);
}

void execute_8086(Arena *arena, u8 *buffer, u64 read_bytes, s32 fd, u32 startFlags)
{
    u8 opcode;

    t_ctx ctx = 
    {
        .b = buffer,
        .ip = 0,
        .seg_prefix = 0xFF,
    };

    Cpu cpu =
    {
        .regs = {0},
        .Memory = {0},
    };

    u16 regsStateOld[14] = {0}; // NOTE: buffer where we store the old state of cpu.regs

    while (ctx.ip < read_bytes)
    {
        u64 scratch_start = arena->pos;
        memcpy(regsStateOld, cpu.regs, 14 * sizeof(u16)); // NOTE: save the old state of cpu.regs
        ctx.b = &buffer[ctx.ip];
        opcode = ctx.b[0];
        func_ptr handler = opcode_table[opcode];
        Instruction inst = handler(&ctx);
        ctx.ip += inst.size;
        cpu.regs[12] = ctx.ip;
        print_instruction(arena, fd, inst);
        execute_instruction(arena, fd, &cpu, inst, regsStateOld, &ctx);
        s32 written = write(fd, "\n", 1);
        if (written == -1) return;
        
        arena->pos = scratch_start;
    }
    print_final_regs(arena, fd, cpu.regs);

    if ((startFlags >> MASK_DUMP) & 1)
    {
        write_memory(cpu.Memory, MegaByte(1));
    }
}