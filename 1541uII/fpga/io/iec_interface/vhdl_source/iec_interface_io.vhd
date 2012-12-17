-------------------------------------------------------------------------------
-- Title      : IEC Interface
-------------------------------------------------------------------------------
-- File       : iec_interface.vhd
-- Created    : 2008-01-17
-- Last update: 2008-01-17
-------------------------------------------------------------------------------
-- Description: This module implements a simple IEC transceiver.
-------------------------------------------------------------------------------
 
-- LUT/FF/S3S/BRAM: 238/ 85/126/0 (not programmable)
-- LUT/FF/S3S/BRAM: 279/104/148/0 (programmable)


library ieee;
    use ieee.std_logic_1164.all;
    use ieee.numeric_std.all;

library work;
    use work.io_bus_pkg.all;

entity iec_interface_io is
generic (
    g_state_readback    : boolean := false;
    programmable_timing : boolean := true );
port (
    clock           : in  std_logic;
    reset           : in  std_logic;

    iec_atn_i       : in  std_logic;
    iec_atn_o       : out std_logic;
    iec_clk_i       : in  std_logic;
    iec_clk_o       : out std_logic;
    iec_data_i      : in  std_logic;
    iec_data_o      : out std_logic;

    io_req          : in  t_io_req;
    io_resp         : out t_io_resp;

    -- debug
    iec_state       : out std_logic_vector(3 downto 0);
    iec_dbg         : out std_logic_vector(1 downto 0) );
    -- end debug
    
end iec_interface_io;


architecture erno of iec_interface_io is
    signal bit_cnt       : integer range 0 to 8;
    signal data_reg      : std_logic_vector(7 downto 0);
    signal send_data     : std_logic_vector(7 downto 0);
    signal delay         : integer range 0 to 255;
    signal clk_d         : std_logic;
    signal atn_d         : std_logic;

    constant c_div       : integer := 250;
    signal timer_div     : integer range 0 to c_div-1 := 0;
    signal timer_tick    : std_logic := '0';

    signal flag_eoi		 : std_logic;
	signal flag_atn		 : std_logic;
    signal flag_data_av  : std_logic;
    signal flag_clr2send : std_logic;
    signal flag_byte_sent: std_logic;
	signal ctrl_data_av  : std_logic;
	signal ctrl_nodata	 : std_logic;
	signal ctrl_talker	 : std_logic;
	signal ctrl_listener : std_logic;
	signal ctrl_eoi		 : std_logic;
    signal ctrl_swready  : std_logic;
	signal ctrl_swreset  : std_logic;
	signal ctrl_killed   : std_logic;
    signal ctrl_address  : std_logic_vector(4 downto 0);
    
    signal clk_edge_seen : std_logic;
	signal clk_edge_d    : std_logic := '1';
	signal atn_edge_d    : std_logic := '1';
	
    type   t_state    is ( idle, prep_recv, reoi_chk, reoi_ack, recv, rack_wait4sw, prep_talk, turn_around,
    						wait4listener, teoi, teoi_ack, trans1, trans2, twait4ack, sw_byte, ack_byte, prep_atn);    						
    						
    signal state        : t_state;

    signal time1        : std_logic_vector(5 downto 0) := "010100";
    signal time2        : std_logic_vector(5 downto 0) := "010100";
    signal time3        : std_logic_vector(5 downto 0) := "010100";
    signal time4        : std_logic_vector(5 downto 0) := "010100";
    
-- comment these two lines when done
--    attribute fsm_encoding : string;
--    attribute fsm_encoding of state : signal is "sequential";

    signal encoded_state : std_logic_vector(3 downto 0) := X"0";
    
begin
    process(clock)
        procedure continue_next is
        begin
			if ctrl_talker = '1' and iec_atn_i = '1' and iec_clk_i = '1' then 
				iec_clk_o <= '0';
			  	iec_data_o <= '1';
			  	state <= prep_talk;
			elsif iec_atn_i = '0' then
				state <= prep_recv;
			elsif ctrl_listener = '1' and flag_eoi='0' then
				state <= prep_recv;
			else
				state <= idle;
			end if;
        end procedure continue_next;

    begin
        if rising_edge(clock) then
            if timer_div = 0 then
                timer_div <= c_div - 1;
                timer_tick <= '1';
            else
                timer_div <= timer_div - 1;
                timer_tick <= '0';
            end if;

            atn_d  <= iec_atn_i;
            clk_d  <= iec_clk_i;

            flag_clr2send <= '0';
            
			if delay /= 0 and timer_tick = '1' then
				delay <= delay - 1;
			end if;

    	    if iec_clk_i='1' and clk_d='0' then
        	    clk_edge_seen <= '1';
        	end if;

            case state is
            when idle =>
                iec_data_o <= '1';
                iec_clk_o  <= '1';

            when prep_recv =>
            	if clk_edge_seen = '1' and ctrl_swready='1' then
            		iec_data_o <= '1';
            		delay <= 40;
            		bit_cnt <= 8;
            		flag_eoi <= '0';
            		state <= reoi_chk;
            	elsif ctrl_talker = '1' and iec_atn_i = '1' then
				  	iec_data_o <= '1';
                    state <= turn_around;
                elsif ctrl_talker = '0' and ctrl_listener = '0' and iec_atn_i = '1' then
                    state <= idle;
            	end if;
            	
            when reoi_chk =>
            	if iec_clk_i = '0' then
					flag_atn <= not iec_atn_i;
            		state <= recv;
            	elsif delay = 0 then 
            		iec_data_o <= '0';
            		delay <= 20;
					flag_eoi <= '1';
            		state <= reoi_ack;
            	end if;
            	
            when reoi_ack =>
            	if iec_clk_i = '0' then
            		iec_data_o <= '1';
					flag_atn <= not iec_atn_i;
            		state <= recv;
            	elsif delay = 0 then
            		iec_data_o <= '1';
            	end if;
            	
            when recv =>
				if iec_clk_i='1' and clk_d='0' then
					bit_cnt <= bit_cnt - 1;
					data_reg <= iec_data_i & data_reg(7 downto 1);
				end if;
				if iec_clk_i='0' and bit_cnt = 0 then -- end of transfer
                    delay <= 20;
                    if flag_atn='1' then
                        if data_reg(7 downto 5)="001" then -- listen
                            ctrl_talker <= '0';
                            ctrl_listener <= '0';
                            if data_reg(4 downto 0) = ctrl_address then
                                ctrl_listener <= '1';
                                state <= ack_byte;
                            end if;
                        elsif data_reg(7 downto 5)="010" then -- talk
                            ctrl_talker <= '0';
                            ctrl_listener <= '0';
                            if data_reg(4 downto 0) = ctrl_address then
                                ctrl_talker <= '1';
                                state <= ack_byte;
                            end if;
                        elsif ctrl_talker='1' or ctrl_listener='1' then -- other atn byte
                            state <= sw_byte;
                        else
                            state <= idle;
                        end if;
                    else -- byte without atn.. we can only be here if we were listener!
                        state <= sw_byte;
                    end if;                        
                end if;

            when ack_byte =>
                iec_data_o <= '0';
                clk_edge_seen <= '0';
                if delay=0 then
                    continue_next;
                end if;                

            when sw_byte =>
				iec_data_o <= '0';
				flag_data_av <= '1';
				ctrl_swready <= '0';
        	    clk_edge_seen <= '0';
				state <= rack_wait4sw;

            when rack_wait4sw =>
				if ctrl_swready = '1' then
                    continue_next;
				end if;									
				
            when prep_talk =>
                flag_clr2send <= '1';
            	if ctrl_data_av = '1' and delay = 0 then
                    data_reg <= send_data; -- copy
                    flag_byte_sent <= '0';
            		iec_clk_o <= '1';
            		bit_cnt <= 8;
            		state <= wait4listener;
            	end if;
            	
            when wait4listener =>
				if iec_data_i = '1' then
					if ctrl_eoi = '1' then
						state <= teoi;
					else
						iec_clk_o <= '0';
						iec_data_o <= data_reg(0);
						delay <= to_integer(unsigned(time1));
						state <= trans1;
					end if;
				end if;
        	
            when teoi =>
            	if iec_data_i = '0' then 
            		state <= teoi_ack;
            	end if;
           
            when teoi_ack =>
            	if ctrl_nodata = '1' then
            		state <= idle;
            	elsif iec_data_i = '1' then
            		iec_clk_o <= '0';
            		iec_data_o <= data_reg(0);
					delay <= to_integer(unsigned(time1));
            		state <= trans1;
            	end if;
            	
            when trans1 =>
				if delay = 0 then 
					iec_clk_o <= '1';
					data_reg <= '1' & data_reg(7 downto 1);
					delay <= to_integer(unsigned(time2));
					state <= trans2;
				end if;
				
			when trans2 =>
    			if delay = 0 then 
	    			iec_data_o <= data_reg(0);
   					iec_clk_o <= '0';
    				if bit_cnt = 1 then    					
						state <= twait4ack;
					else 
						bit_cnt <= bit_cnt - 1;	    					
						delay <= to_integer(unsigned(time3));
						state <= trans1;
					end if;
				end if;

            when twait4ack =>
            	if iec_data_i = '0' and timer_tick='1' then 
					ctrl_data_av <= '0';
					flag_byte_sent <= '1';
            		if ctrl_eoi = '0' then
            			state <= prep_talk;
            		else
	   					state <= idle;
	   					ctrl_talker <= '0';
	   				end if;
	   			end if;
            			
            when turn_around =>
                if iec_clk_i='1' then
					iec_clk_o <= '0';
					delay     <= 20;
				  	state <= prep_talk;
                end if;
                                    
            when prep_atn =>
                if timer_tick='1' then
                    clk_edge_seen <= '0';
            		state <= prep_recv;
                end if;
                    
            
            when others =>
                null;

            end case;            

            if ctrl_killed = '0' then
            	if iec_atn_i = '0' and atn_d = '1' then -- falling edge attention
            		iec_data_o    <= '0';
            		iec_clk_o     <= '1';
            		ctrl_data_av  <= '0';
                    clk_edge_seen <= '0';

                    if timer_tick='0' then
                        atn_d <= '1'; -- delay edge detection until clock is also seen high
                    else
                		state <= prep_atn;
                    end if;
                end if;
            end if;
            
            -- BUS INTERFACE
            io_resp <= c_io_resp_init;
			if io_req.write='1' then
			    io_resp.ack <= '1';
                if io_req.address(2)='0' then
    				case io_req.address(1 downto 0) is
    				when "00" =>
    					send_data <= io_req.data;
    
    				when "01" =>
                        if io_req.data(7)='1' then
                           flag_data_av  <= '0';
                        end if;
    					ctrl_swready  <= io_req.data(7);
    					ctrl_nodata   <= io_req.data(6);
--    					ctrl_talker   <= io_req.data(5);
--    					ctrl_listener <= io_req.data(4);
    					ctrl_data_av  <= io_req.data(3);
    					ctrl_swreset  <= io_req.data(2);
    					ctrl_killed   <= io_req.data(1);
    					ctrl_eoi      <= io_req.data(0);

                    when "10" =>
                        ctrl_address  <= io_req.data(4 downto 0);
                    
                    when others =>
                        null;
                    end case;
                elsif programmable_timing then -- note: cpu_addr(2) = '1'
                    case io_req.address(1 downto 0) is
                    when "00" =>
                        time1 <= io_req.data(5 downto 0);
    
                    when "01" =>
                        time2 <= io_req.data(5 downto 0);
    
                    when "10" =>
                        time3 <= io_req.data(5 downto 0);
    
                    when "11" =>
                        time4 <= io_req.data(5 downto 0);
    
    				when others =>
    					null;
    				end case;
                end if;

            elsif io_req.read='1' then
                io_resp.ack <= '1';
        		case io_req.address(1 downto 0) is
        		when "00" =>
        			io_resp.data <= data_reg;
        
        		when "01" =>
        			io_resp.data(7) <= flag_data_av;
        			io_resp.data(1) <= flag_atn;
        			io_resp.data(5) <= ctrl_talker;
        			io_resp.data(4) <= ctrl_listener;
        			io_resp.data(3) <= flag_clr2send;
        			io_resp.data(2) <= flag_byte_sent;
        			io_resp.data(0) <= flag_eoi;
        
                when "11" =>
                    io_resp.data(3 downto 0) <= encoded_state;
        
        		when others =>
        			null;
        		end case;

			end if;

            if reset='1' or ctrl_swreset='1' then
            	state <= idle;
        		iec_data_o <= '1';
        		iec_clk_o  <= '1';

        	    clk_edge_seen  <= '0';
			    flag_eoi       <= '0';
				flag_atn       <= '0';
			    flag_data_av   <= '0';
                flag_clr2send  <= '0';
                flag_byte_sent <= '0';
				ctrl_swreset   <= '0';
				ctrl_swready   <= '0';
				ctrl_data_av   <= '0';
				ctrl_nodata	   <= '0';
				ctrl_talker	   <= '0';
				ctrl_listener  <= '0';
				ctrl_eoi       <= '0';
                ctrl_killed    <= '1';
                ctrl_address   <= "01001"; -- 9
			end if;
        end if;
    end process;

    r_state: if g_state_readback generate
        with state select encoded_state <=
            X"0" when idle,
            X"1" when prep_recv,
            X"2" when reoi_chk,
            X"3" when reoi_ack,
            X"4" when recv,
            X"5" when rack_wait4sw,
            X"6" when prep_talk,
            X"7" when wait4listener,
            X"8" when teoi,
            X"9" when teoi_ack,
            X"A" when trans1,
            X"B" when trans2,
            X"C" when twait4ack,
            X"D" when sw_byte,
            X"E" when ack_byte,
            X"F" when others;    						
    end generate;

    iec_atn_o <= '1';
    
    iec_state  <= encoded_state;
    iec_dbg(0) <= ctrl_data_av;
    iec_dbg(1) <= ctrl_swready;

end erno;

