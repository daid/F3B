if setSignNr == nil then function setSignNr() end end

function resolveButton(button)
	p = 0
	t = -1
	if button >= 4 and button <= 15 then
		if button % 2 == 0 then
			--Normal button station A
			t = 0
			p = (14 - button) / 2
		else
			--Reset button station A
			t = 2
			p = (15 - button) / 2
		end
	end
	if button >= 23 and button <= 31 then
		if button % 2 == 0 then
		else
			--Normal button station B
			t = 1
			p = (31 - button) / 2
		end
	end
	if button == 2 then
		t = 4	--start
	end
	if button == 3 then
		t = 3	--reset
	end
	return p + 1, t
end

function getButton(p, t)
	p = p - 1
	but = -1
	if t == 0 then
		but = 14 - p * 2
	elseif t == 2 then
		but = 15 - p * 2
	elseif t == 1 then
		but = 31 - p * 2
	elseif t == 3 then
		but = 3 --reset
	elseif t == 4 then
		but = 2 --start
	end
	if (but < 0) then
		return 0
	end
	return getButtonState(but)
end

function doSignal(number)
    if number == -1 or number == -2 then
        startSignal(3, 1500)
		startSignal(11, 1500)
		startSignal(12, 1500)
		startSignal(13, 1500)
		startSignal(14, 1500)
		startSignal(15, 1500)
    end
	
	if number >= 0 and number <= 5 then
		startSignal(8 - number, 1500)
		startSignal(16 - number, 1500)
	end
	if number >= 0 + 16 and number <= 5 + 16 then
		startSignal(32 - number, 1500)
	end
end

function formatFullTime(time)
	if time < 0 then
		return "-"
	end
	return string.format("%02i.%03i", time / 1000, time % 1000);
end

function setClockTime(totalTime, nr)
	local s0 = 0x03;
	local s1 = 0x02;
	local s2 = 0x01;
	local s3 = 0x00;
	
    local totalSec = ((totalTime - getTime()) / 1000) + 1;
    local totalMin = totalSec / 60;
    totalSec = totalSec - (totalMin * 60);
    setSignNr(s0, totalSec % 10);
    setSignNr(s1, totalSec / 10);
    setSignNr(s2, totalMin % 10);
	if totalMin > 9 then
		setSignNr(s3, totalMin / 10);
	else
		setSignNr(s3, 10);
	end
end
function setLedClockTime(totalTime, nr)
	local s0 = 0x16;
	local s1 = 0x17;
	local s2 = 0x18;
	local s3 = 0x19;
	
	if nr == 1 then s0 = 0x11; s1 = 0x10; s2 = 0x12; s3 = 0x13; end
	
    local totalSec = ((totalTime - getTime()) / 1000) + 1;
    local totalMin = totalSec / 60;
    totalSec = totalSec - (totalMin * 60);
	
    setSignNr(s0, totalSec % 10);
    setSignNr(s1, totalSec / 10);
    setSignNr(s2, totalMin % 10);
	if totalMin > 9 then
		setSignNr(s3, totalMin / 10);
	else
		setSignNr(s3, 10);
	end
end
function setClock60Speed(timeLeft)
	if timeLeft < 0 then
		setSignNr(0x20, 10);
		setSignNr(0x21, 10);
		return
	end
	setSignNr(0x20, timeLeft % 10);
	setSignNr(0x21, timeLeft / 10);
end
function setLedClockTimeSpeed(speedTime, nr)
	local s0 = 0x16;
	local s1 = 0x17;
	local s2 = 0x18;
	local s3 = 0x19;
	
	if nr == 1 then s0 = 0x11; s1 = 0x10; s2 = 0x12; s3 = 0x13; end
	
	local totalSec = speedTime / 1000;
	local totalHSec = (speedTime / 10) % 100;

    setSignNr(s0, totalHSec % 10);
    setSignNr(s1, totalHSec / 10);
    setSignNr(s2, totalSec % 10);
	if totalSec > 9 then
		setSignNr(s3, totalSec / 10);
	else
		setSignNr(s3, 10);
	end
end
function setScoreSign(p, nr)
	local s0 = 0x30;
	local s1 = 0x31;
	local s2 = 0x32;
	if p == 1 then s0 = 0x44; s1 = 0x49; s2 = 0x48; end
	if p == 2 then s0 = 0x47; s1 = 0x46; s2 = 0x43; end
	if p == 3 then s0 = 0x42; s1 = 0x41; s2 = 0x40; end
	if p == 4 then s0 = 0x30; s1 = 0x31; s2 = 0x32; end
	if p == 5 then s0 = 0x33; s1 = 0x36; s2 = 0x37; end
	if p == 6 then s0 = 0x38; s1 = 0x39; s2 = 0x35; end
	
	if nr < 0 then
		setSignNr(s0, 10);
		setSignNr(s1, 10);
		setSignNr(s2, 10);
	else
		setSignNr(s0, 10 + p);
		if nr > 9 then
			setSignNr(s1, nr / 10);
		else
			setSignNr(s1, 10);
		end
		if nr > 0 then
			setSignNr(s2, nr % 10);
		else
			setSignNr(s2, 10);
		end
	end
end

playerCount = 5
--Full 16 players so we can access all array points.
playerStatus = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
playerLaps = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
playerStartTime = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

for i = 0, 0xFF, 1 do
    setSignNr(i, 0x200);
end

getTime(0); -- Reset any time that might be running
setState("TEXT:Setting up equipment for next flight");
setAdminState("BUTTON:Duration|BUTTON:Distance|BUTTON:Speed")
gameType = 0
while gameType == 0 do
	sleep(100);
	button, time = getButtonEvent();
	if button ~= nil then
		p, t = resolveButton(button)
		if t == 3 then --reset
			return
		end
	end
	action = getWebAction()
	if action ~= nil then
		if action == "Duration" then
			gameType = 1
		end
		if action == "Distance" then
			gameType = 2
		end
		if action == "Speed" then
			gameType = 3
		end
	end
end
clearButtonEvents();
setAdminState("BUTTON:Start")

if gameType == 1 then
	getTime(1);
	startTime1 = -1;
	startTime2 = -1;
	--Wait for operator to start the round
	setAdminState("BUTTON:Start|BUTTON:Reset")
	while true do
		sleep(10)
		state = "TEXT:Duration flight"
		if startTime1 > -1 then
			setLedClockTime(DURATION_WORK_TIME + startTime1, 0)
			if (DURATION_WORK_TIME + startTime1) - getTime() < 0 then
				startTime1 = -1
				setLedClockTime(getTime()-1000, 0)
				doSignal(5)
			end
			state = state .. "|WORKTIME:" .. DURATION_WORK_TIME + startTime1
		end
		if startTime2 > -1 then
			setLedClockTime(DURATION_WORK_TIME + startTime2, 1)
			if (DURATION_WORK_TIME + startTime2) - getTime() < 0 then
				startTime2 = -1
				setLedClockTime(getTime()-1000, 1)
				doSignal(1)
			end
			state = state .. "|WORKTIME:" .. DURATION_WORK_TIME + startTime2
		end
		-- state = state .. "|BUTTON:Start"
		setState(state);
		button, time = getButtonEvent();
		action = getWebAction()
		if action == "Start" then
			button = 2
			time = getTime()
		end
		if action == "Reset" then
			return
		end
		while button ~= nil do
			p, t = resolveButton(button);
			if t == 4 then
				--Start button
				if startTime2 < startTime1 then
					startTime2 = time
					doSignal(1)
				else
					startTime1 = time
					doSignal(5)
				end
			end
			if t == 3 then
				--full reset
				return
			end
			
			button, time = getButtonEvent();
		end
	end
end
if gameType == 2 then
	setClockTime(DISTANCE_WORK_TIME + getTime() - 1, 0);
	setLedClockTime(DISTANCE_WORK_TIME + getTime() - 1, 0);
	log("Waiting for start of distance flight");
	setState("WORKTIME:"..DISTANCE_WORK_TIME.."|TEXT:Wait for flight to start");
	--Wait for operator to start the round
	started = false
	while not started do
		sleep(100)
		button, time = getButtonEvent();
		while button ~= nil do
			p, t = resolveButton(button);
			if t == 4 then
				started = true
			end
			
			button, time = getButtonEvent();
		end
		action = getWebAction();
		if action == "Start" then
			started = true
		end
	end
	
	for i=1,5,1 do
		setScoreSign(i, 0);
	end
	--Start signal
	clearButtonEvents();
	now = getTime(1);
	doSignal(-1)
	start = now;
	log("Round started");
	setAdminState("BUTTON:Reset")
	--playerStatus = {0, 0, 0, 0, 0}; --Reset player status to "landed, wait for B"
	--[[
	Status:
	0: landed wait for B
		1: landed wait for A
		2: flying, waiting to exit field.
		3: flying, waiting for point A first time
		4: flying towards B
		5: flying towards A
	--]]
	
	--Main round logic
	while started and now < start + DISTANCE_WORK_TIME do
        setClockTime(DISTANCE_WORK_TIME, 0);
		setLedClockTime(DISTANCE_WORK_TIME, 0);
		sleep(100);
		
		button, time = getButtonEvent();
		while button ~= nil do
			p, t = resolveButton(button);
			if playerStatus[p] == 0 and t == 1 then
				--Takeoff
				playerLaps[p] = 0
				setScoreSign(p, playerLaps[p]);
				playerStatus[p] = 1
				doSignal(p);
				log("Player "..p.." B seen at start");
			elseif playerStatus[p] == 1 and t == 0 then
				--Takeoff
				playerStatus[p] = 2
				doSignal(p);
				log("Player "..p.." A seen at start");
			elseif playerStatus[p] == 2 and t == 0 then
				--Flying out of the field.
				playerStatus[p] = 3
				doSignal(p);
				log("Player "..p.." A exit field after start");
			elseif playerStatus[p] == 3 and t == 0 then
				--Takeoff
				playerStatus[p] = 4
				playerLaps[p] = 0
				playerStartTime[p] = time
				doSignal(p);
				log("Player "..p.." A enter field, start of laps");
			elseif playerStatus[p] > 0 and t == 2 then
				--Restart
				playerStatus[p] = 0
				playerStartTime[p] = 0
				log("Player "..p.." reset");
			elseif playerStatus[p] == 4 and t == 1 and time < playerStartTime[p] + DISTANCE_FLY_TIME then
				--Pass at station B
				playerStatus[p] = 5
				playerLaps[p] = playerLaps[p] + 1
				setScoreSign(p, playerLaps[p]);
				doSignal(p);
				log("Player "..p.." pass B, lap " .. playerLaps[p]);
			elseif playerStatus[p] == 5 and t == 0 and time < playerStartTime[p] + DISTANCE_FLY_TIME then
				--Pass at station A
				playerStatus[p] = 4
				playerLaps[p] = playerLaps[p] + 1
				setScoreSign(p, playerLaps[p]);
				doSignal(p);
				log("Player "..p.." pass A, lap " .. playerLaps[p]);
			elseif t == 3 then
				--reset
				log("Reset button pressed");
				started = false
			end
			
			button, time = getButtonEvent();
		end
		action = getWebAction()
		if action == "Reset" then
			log("Reset button pressed");
			started = false
		end
		
		state = "WORKTIME:" .. DISTANCE_WORK_TIME;
		s = "|TABLE:Player;Laps;Flight time;"
		for i = 1, playerCount, 1 do
			s = s .. ":" .. string.char(i+64) .. ";" .. playerLaps[i]
			if playerStatus[i] == 0 then
				s = s .. ";-"
				s = s .. ";waiting for B at takeoff"
			elseif playerStatus[i] == 1 then
				s = s .. ";-"
				s = s .. ";waiting for A at takeoff"
			elseif playerStatus[i] == 2 then
				s = s .. ";-"
				s = s .. ";waiting till exit field at A"
			elseif playerStatus[i] == 3 then
				s = s .. ";-"
				s = s .. ";waiting for entry at A "
			elseif now >= playerStartTime[i] + DISTANCE_FLY_TIME then
				s = s .. ";-"
				s = s .. ";time is up"
			elseif playerStatus[i] == 4 then
				s = s .. ";#T#" .. (playerStartTime[i] + DISTANCE_FLY_TIME)
				s = s .. ";flying to B"
			elseif playerStatus[i] == 5 then
				s = s .. ";#T#" .. (playerStartTime[i] + DISTANCE_FLY_TIME)
				s = s .. ";flying to A"
			else
				s = s .. ";-"
				s = s .. ";?"
			end
		end
		state = state .. s
		setState(state)
		
		now = getTime();
	end

	s = "TABLE:Player;Laps"
	for i = 1, playerCount, 1 do
		s = s .. "|" .. string.char(i+64) .. ";" .. playerLaps[i]
	end
	setState("TEXT:Flight has ended|"..s)
	
	--End signal
	setClockTime(getTime()-1000);
	setLedClockTime(getTime()-1000, 0);
	log("Distance flight end");
	getTime(0);
	doSignal(-2);
	
	while started do
		sleep(100);
		--Waiting for reset.
		button, time = getButtonEvent();
		while button ~= nil do
			p, t = resolveButton(button);
			if t == 3 then
				--reset
				started = false
			end
			
			button, time = getButtonEvent();
		end
	end
end
if gameType == 3 then
	while true do
		setClockTime(SPEED_WORK_TIME + getTime() - 1, 0);
		setLedClockTime(SPEED_WORK_TIME + getTime() - 1, 0);
		setClock60Speed(-1);
		setState("WORKTIME:"..SPEED_WORK_TIME.."|TEXT:Wait for round to start");
		setAdminState("BUTTON:Start")
		setAdminState("TEXTENTRY:Speed round X, flight X:Start")
		--Before the 3 minutes work time. Wait for start button.
		started = false
		while not started do
			sleep(100);
			button, time = getButtonEvent();
			while button ~= nil do
				p, t = resolveButton(button);
				if t == 3 then
					--reset
					return
				elseif t == 4 then --start
					started = true
				end
				button, time = getButtonEvent();
			end
			action = getWebAction()
			if action ~= nil then
				local i = string.find(action, "&")
				local param = ""
				if i ~= nil then
					param = string.sub(action, i + 1)
					param = string.gsub(param, "%%20", "_")
					action = string.sub(action, 0, i - 1)
				end
				if action == "Start" then
					started = true
				end
			end
		end
		--Start signal
		clearButtonEvents();
		now = getTime(1);
		doSignal(-1);
		start = now;
		log("Speed round started");
		setAdminState("BUTTON:Reset")
		flightStart = -1;
		lastflighttime = -1;
		lastEventTime = -1;
		speed60Time = -1;
		
		state = 0;
		while now < start + SPEED_WORK_TIME do
            setClockTime(SPEED_WORK_TIME, 0);
			setLedClockTime(SPEED_WORK_TIME, 0);
			if speed60Time > -1 then
				if now - speed60Time > 60*1000 then
					setClock60Speed(0);
					if state < 10 then
						state = 20
						doSignal(3);
					end
				else
					setClock60Speed(60 - (now - speed60Time) / 1000);
				end
			else
				setClock60Speed(60);
			end
			sleep(10);
			action = getWebAction();
			if action == "Reset" then
				return
			end
			button, time = getButtonEvent();
			while button ~= nil do
				p, t = resolveButton(button);
				if t == 3 then
					--global reset
					return
				end
				event = -1
				if lastEventTime == -1 or time - lastEventTime > 1000 then
					--if (t == 0 and p == 1 and getButton(2, 0) ~= 0) or (t == 0 and p == 2 and getButton(1, 0) ~= 0) then
						--Station A pressed both
					if (t == 0 and p == 1) or (t == 0 and p == 2) then
						--Station A pressed
						event = 1
						lastEventTime = time
					end
					--if (t == 1 and p == 1 and getButton(2, 1) ~= 0) or (t == 1 and p == 2 and getButton(1, 1) ~= 0) then
						--Station B pressed both
					if (t == 1 and p == 1) or (t == 1 and p == 2) then
						--Station B pressed
						event = 2
						lastEventTime = time
					end
				end
				if t == 2 and (p == 1 or p == 2) and (state == 0 or state == 1 or state == 20) then
					--Relaunch
					state = 0;
					flightStart = -1
					speed60Time = -1
				end
				if p == 5 and speed60Time == -1 and state < 10 then
					speed60Time = now
				end
				if state == 0 then
					if event == 1 then
						doSignal(1);
						state = 1;
					end
				elseif state == 1 then
					if event == 1 then
						doSignal(1);
						speed60Time = -1
						state = 10;
						flightStart = time
						setLedClockTimeSpeed(time - flightStart, 1);
					end
				elseif state == 10 then
					if event == 2 then
						--1x 150m
						doSignal(1);
						state = 11;
						setLedClockTimeSpeed(time - flightStart, 1);
					end
				elseif state == 11 then
					if event == 1 then
						--2x 150m
						doSignal(1);
						state = 12;
						setLedClockTimeSpeed(time - flightStart, 1);
					end
				elseif state == 12 then
					if event == 2 then
						--3x 150m
						doSignal(1);
						state = 13;
						setLedClockTimeSpeed(time - flightStart, 1);
					end
				elseif state == 13 then
					if event == 1 then
						--4x 150m
						setLedClockTimeSpeed(time - flightStart, 1);
						lastflighttime = time - flightStart;
						--setLedClockTimeSpeed(lastflighttime, 1);
						start = getTime(1);
						lastEventTime = start;
						doSignal(1);
						clearButtonEvents();
						state = 0;
						flightStart = -1
					end
				end
				button, time = getButtonEvent();
			end
			
			if state == 0 then
				s = "TEXT:Waiting for exit of field at A"
			elseif state == 1 then
				s = "TEXT:Waiting for entry of field at A"
			elseif state == 10 then
				s = "TEXT:Flying to B (0m)"
			elseif state == 11 then
				s = "TEXT:Flying to A (150m)"
			elseif state == 12 then
				s = "TEXT:Flying to B (300m)"
			elseif state == 13 then
				s = "TEXT:Flying to A (450m)"
			elseif state == 20 then
				s = "TEXT:Out of 60 seconds, need to land"
			else
				s = "TEXT:?"
			end
			if speed60Time ~= -1 and state ~= 20 then
				s = s .. " (need to start within " .. 60 - ((now - speed60Time) / 1000) .. ")"
			end
			s = s .. "|TEXT:Flight time "
			if flightStart == -1 then
				s = s .. "-";
			else
				s = s .. formatFullTime(now - flightStart);
			end
			s = s .. "|TEXT:Previous flight  " .. formatFullTime(lastflighttime);
			setState("WORKTIME:"..SPEED_WORK_TIME.."|"..s);
			
			now = getTime();
		end
		
		--End signal
		setClockTime(getTime()-1);
		setLedClockTime(getTime()-1, 0);
		log("Out of worktime");
		getTime(0);
		doSignal(-2);
	end
end
