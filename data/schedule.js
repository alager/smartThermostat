

var monday;
var tuesday;
var wednesday;
var thursday;
var friday;
var saturday;
var sunday;

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
	monday = document.getElementById( "monday" );		//.addEventListener( "click", editSched );
	tuesday = document.getElementById( "tuesday" );	//	.addEventListener( "click", editSched );
	wednesday = document.getElementById( "wednesday" );	//.addEventListener( "click", editSched );
	thursday = document.getElementById( "thursday" );	//.addEventListener( "click", editSched );
	friday = document.getElementById( "friday" );		//.addEventListener( "click", editSched );
	saturday = document.getElementById( "saturday" );	//.addEventListener( "click", editSched );
	sunday = document.getElementById( "sunday" );		//.addEventListener( "click", editSched );

	document.getElementById( "scheduleBox" ).addEventListener( "click", editSched );

	schedBox.addEventListener( 'click', editSched );
	buttonHeat.addEventListener( 'click', addHeat );
	buttonCool.addEventListener( 'click', addCool );
	// buttonDelete.addEventListener( 'click', editSched );
}


function editSched()
{
	// find the day div if the hour target was clicked
	var myDiv = event.target;
	// console.log( myDiv.id );

	if( myDiv.id.charAt( 3 ) == '-' )
	{
		// we need to find the parent
		myDiv = myDiv.parentNode;
		// console.log( "new Div: " + myDiv.id );
	}

	// hide all the divs and then only show the one that was clicked
	hideDayDivs();
	myDiv.hidden = false;

	// make this div large
	myDiv.classList.add( "dayBig" );
	myDiv.classList.remove( "daySmall" );

}

function hideDayDivs(  )
{
	monday.hidden = true;
	tuesday.hidden = true;
	wednesday.hidden = true;
	thursday.hidden = true;
	friday.hidden = true;
	saturday.hidden = true;
	sunday.hidden = true;
}

function addHeat()
{
	// var dotSpan = document.createElement('span');
	var dotSpan = document.getElementById( 'mon-am-00' );
	dotSpan.classList.add( "dot");

	// var dayDiv = document.getElementById( "monday" );
	// dayDiv.appendChild( dotSpan );
}

function addCool()
{

}