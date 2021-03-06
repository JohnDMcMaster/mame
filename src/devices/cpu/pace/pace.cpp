// license:BSD-3-Clause
// copyright-holders:AJR
/***************************************************************************

    National Semiconductor PACE/INS8900

    The IPC-16A PACE (Processing and Control Element) was one of the first
    commercially available 16-bit microprocessors. It was a successor to
    the multiple-chip IMP-16 processor and implemented a similar four-
    accumulator architecture in a single PMOS LSI package using internal
    microcode. PACE requires two clock inputs with non-overlapping low
    phases and frequencies between 1.25 and 1.538 MHz.

    The standard machine cycle takes 4 clock periods (and the shortest
    instructions take 4 cycles each), though cycles can be stretched by
    asserting the EXTEND pin. Six interrupts are available, one triggered
    only by internal stack full/empty conditions; the nonmaskable level 0
    interrupt is intended primarily for debugging. The on-chip 10-level
    LIFO stack may hold both return addresses and register data. All
    instructions are single-word.

    INS8900 was a NMOS reimplementation of PACE which takes a single-phase
    clock and allows up to 2 MHz operation. It has different power supply
    requirements, but is functionally identical.

***************************************************************************/

#include "emu.h"
#include "pace.h"
#include "pacedasm.h"


//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

// device type definition
DEFINE_DEVICE_TYPE(INS8900, ins8900_device, "ins8900", "National Semiconductor INS8900")


//**************************************************************************
//  DEVICE CONSTRUCTION AND INITIALIZATION
//**************************************************************************

//-------------------------------------------------
//  pace_device - constructor
//-------------------------------------------------

pace_device::pace_device(const machine_config &mconfig, device_type type, const char *tag, device_t *owner, u32 clock)
	: cpu_device(mconfig, type, tag, owner, clock)
	, m_space_config("program", ENDIANNESS_LITTLE, 16, 16, -1)
	, m_space(nullptr)
	, m_inst_cache(nullptr)
	, m_jc_callback{{*this}, {*this}, {*this}}
	, m_flag_callback{{*this}, {*this}, {*this}, {*this}}
	, m_icount(0)
	, m_pc(0)
	, m_fr(0xffff)
	, m_ac{0, 0, 0, 0}
	, m_sp(0)
	, m_stk{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
{
}


//-------------------------------------------------
//  ins8900_device - constructor
//-------------------------------------------------

ins8900_device::ins8900_device(const machine_config &mconfig, const char *tag, device_t *owner, u32 clock)
	: pace_device(mconfig, INS8900, tag, owner, clock)
{
}


//-------------------------------------------------
//  memory_space_config - return a vector of
//  configuration structures for memory spaces
//-------------------------------------------------

device_memory_interface::space_config_vector pace_device::memory_space_config() const
{
	return space_config_vector {
		std::make_pair(AS_PROGRAM, &m_space_config)
	};
}


//-------------------------------------------------
//  device_resolve_objects - resolve objects that
//  may be needed for other devices to set
//  initial conditions at start time
//-------------------------------------------------

void pace_device::device_resolve_objects()
{
	// resolve callbacks
	for (auto &cb : m_jc_callback)
		cb.resolve_safe(0);
	for (auto &cb : m_flag_callback)
		cb.resolve_safe();
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void pace_device::device_start()
{
	// get memory spaces
	m_space = &space(AS_PROGRAM);
	m_inst_cache = m_space->cache<1, -1, ENDIANNESS_LITTLE>();

	set_icountptr(m_icount);

	// debug state registration
	state_add(PACE_PC, "PC", m_pc);
	state_add(STATE_GENPC, "GENPC", m_pc).noshow();
	state_add(STATE_GENPCBASE, "GENPCBASE", m_pc).noshow();
	state_add<u16>(PACE_FR, "FR", [this]() { return m_fr; }, [this](u16 data) { fr_w(data); });
	state_add(STATE_GENFLAGS, "GENFLAGS", m_fr).noshow();
	for (int i = 0; i < 4; i++)
		state_add(PACE_AC0 + i, string_format("AC%d", i).c_str(), m_ac[i]);
	state_add<u8>(PACE_SP, "SP",
		[this]() { return m_sp; },
		[this](u8 data) { m_sp = data < 10 ? data : BIT(data, 0) ? 9 : 0; }).mask(0xf);
	for (int i = 0; i < 10; i++)
		state_add(PACE_STK0 + i, string_format("STK%d", i).c_str(), m_stk[i]);

	// save states
	save_item(NAME(m_pc));
	save_item(NAME(m_fr));
	save_item(NAME(m_ac));
	save_item(NAME(m_sp));
	save_item(NAME(m_stk));
}


//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void pace_device::device_reset()
{
	m_pc = 0;
	fr_w(0);
	m_sp = 0;
}


//-------------------------------------------------
//  create_disassembler - factory method for
//  disassembling program code
//-------------------------------------------------

std::unique_ptr<util::disasm_interface> pace_device::create_disassembler()
{
	return std::make_unique<pace_disassembler>();
}


//**************************************************************************
//  I/O LINES, INTERRUPTS AND FLAGS
//**************************************************************************

//-------------------------------------------------
//  fr_w - update the flag register
//-------------------------------------------------

void pace_device::fr_w(u16 data)
{
	for (int i = 0; i < 4; i++)
		if (BIT(data, 11 + i) != BIT(m_fr, 11 + i))
			m_flag_callback[i](BIT(data, 11 + i));

	// Bits 0 and 15 are always logic 1
	m_fr = data | 0x8001;
}


//-------------------------------------------------
//  execute_set_input -
//-------------------------------------------------

void pace_device::execute_set_input(int irqline, int state)
{
	 // TODO
}


//**************************************************************************
//  PROGRAM EXECUTION
//**************************************************************************

//-------------------------------------------------
//  execute_run -
//-------------------------------------------------

void pace_device::execute_run()
{
	// TODO

	debugger_instruction_hook(m_pc);
	m_icount = 0;
}
