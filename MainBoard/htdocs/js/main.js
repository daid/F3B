var currentTime = new Date();
var timeNowMillis = 0;

function formatTime(time)
{
	time = Math.ceil(time / 1000);
	seconds = time % 60;
	minutes = Math.floor(time / 60)
	if (seconds < 10)
		return minutes + ":0" + seconds;
	return minutes + ":" + seconds;
}

function onButtonClick()
{
	$.get('/action?'+$(this).text());
}

function onTextButtonClick()
{
	var id = $(this).text();
	$.get('/action?'+$(this).text()+'&'+$("#text_" + id).val());
}

function fillDiv(div, text)
{
	if (div.attr('previousState') == text)
		return
	div.attr('previousState', text)

	var data = text.split("|");
	
	div.empty()
	for(var idx=0; idx<data.length;idx++)
	{
		var info = data[idx].split(':')
		switch(info[0])
		{
		case 'TEXT':
			div.append($('<div/>').addClass("ui-widget ui-widget-content ui-corner-all box").text(info[1]));
			break;
		case 'BUTTON':
			div.append($('<button/>').text(info[1]).button().click(onButtonClick));
			break;
		case 'TIME':
			timeNowMillis = parseInt(info[1]);
			break;
		case 'WORKTIME':
			totalWorkTime = parseInt(info[1]);
			div.append($('<div/>').addClass("clock ui-widget-content ui-corner-all box").text(formatTime(totalWorkTime - timeNowMillis)));
			break;
		case 'TEXTENTRY':
			div.append($('<input type="text" id="text_'+info[2]+'"/>').addClass("ui-corner-all").val(info[1]));
			div.append($('<button/>').text(info[2]).button().click(onTextButtonClick));
			break;
		case 'TABLE':
			var table = $('<table/>');
			for(var rowIdx=1; rowIdx<info.length; rowIdx++)
			{
				var row = $('<tr/>');
				var colInfo = info[rowIdx].split(';');
				for(var colIdx=0; colIdx<colInfo.length; colIdx++)
				{
					var text = colInfo[colIdx];
					if (text.indexOf("#T#") == 0)
					{
						text = formatTime(parseInt(text.substring(3)) - timeNowMillis);
					}
					if (rowIdx == 1)
						row.append($('<th/>').text(text));
					else
						row.append($('<td/>').text(text));
				}
				table.append(row);
			}
			div.append($('<div/>').addClass("ui-widget ui-widget-content ui-corner-all box").append(table));
			break;
		default:
			div.append($('<div/>').addClass("ui-widget ui-widget-content ui-corner-all ui-state-highlight box").text(data[idx]));
			break;
		}
	}
}