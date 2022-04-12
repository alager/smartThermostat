
// Add event for when the DOM is loaded
// start the javascript from here
document.addEventListener("DOMContentLoaded", function(event)
{
	window.scrollTo(0, 1);
	// document.documentElement.requestFullscreen();
	// screen.orientation.lock('landscape');
	//
	initButtons();
});

// make the scheduleBox clickable
function initButtons()
{
	const schedBox = document.getElementById( "scheduleBox" );

	schedBox.addEventListener( 'click', editSched );
	buttonHeat.addEventListener( 'click', addHeat );
	buttonCool.addEventListener( 'click', addCool );
	buttonDelete.addEventListener( 'click', editSched );
}


function editSched()
{

}

function addHeat()
{
	var dotSpan = document.createElement('span');
	dotSpan.classList.add( "dot");

	var dayDiv = document.getElementById( "monday" );
	dayDiv.appendChild( dotSpan );
}

function addCool()
{

}