# Fix handedness of gloves created by custom reaction


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
