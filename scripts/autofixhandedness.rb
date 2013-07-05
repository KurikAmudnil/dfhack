# autofixhandedness [start|stop|end|status] ; Autofix handedness of gloves created by custom reaction

class AutoFixHandedness

	def initialize
		@running = false
	end

	def process
		return false unless @running

		gloves=df.world.items.other[:GLOVES]	# the colection of GLOVES
		hand=0 # which hand  0=right, 1=left
		count=0
		gloves.each do |glove|
			# if glove is unhanded (right and left = false, fix it)
			unless glove.handedness[0] or glove.handedness[1]
				glove.handedness[hand] = true	# enable hand
				hand ^= 1	# switch hand for next glove
							# XOR assignment: 0 ^ 1 = 1 ; 1 ^ 1 = 0
				count += 1
			end
		end

		puts "Fixed #{count} unhanded glove(s)." unless count == 0
		
	end
	
	def start
		# 2400 = once every 2 dwarf days ; 1200 = once every dwarf day
		@onupdate = df.onupdate_register('autofixhandedness', 2400) { process }
		@running = true
		@item_next_id = 0
	end
	
	def stop
		puts @onupdate
		df.onupdate_unregister(@onupdate)
		@running = false
	end
	
	def status
		@running ? 'autofixhandedness: Running' : 'autofixhandedness: Stopped'
	end
		
end	

case $script_args[0]
when 'start'
    $AutoFixHandedness = AutoFixHandedness.new unless $AutoFixHandedness
    $AutoFixHandedness.start
	puts $AutoFixHandedness.status

when 'end', 'stop'
    $AutoFixHandedness.stop
	puts $AutoFixHandedness.status
else
    if $AutoFixHandedness
        puts $AutoFixHandedness.status
    else
        puts 'Autofix Handedness not loaded.  To start it, use: autofixhandedness start'
    end
end
