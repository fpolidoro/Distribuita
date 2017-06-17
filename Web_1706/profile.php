<?php
	require 'functions.php';
	isUserAuthenticated(true);

	$conn = dbConnect();
?>
<!DOCTYPE html>
<html>
<title>Bid$ - My Offers</title>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="stylesheet" href="https://www.w3schools.com/w3css/4/w3.css">
<link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Montserrat">
<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css">
<style>
body, h1,h2,h3,h4,h5,h6 {font-family: "Montserrat", sans-serif}
.w3-row-padding img {margin-bottom: 12px}
/* Set the width of the sidebar to 120px */
.w3-sidebar {width: 120px;/*background: #222;*/}
.w3-hover-text-indigo:hover{color:#3f51b5!important}
.w3-indigo-extra-light {color:#000 !important; background-color:#f3f4fb !important}
.w3-indigo-light {color:#fff !important; background-color:#8995d6 !important}
.w3-hover-indigo-light:hover{color:#fff !important; background-color:#8995d6 !important}
.hidden{display: none;}
/* Add a left margin to the "page content" that matches the width of the sidebar (120px) */
#main {margin-left: 120px; margin-top: 20px;}
#iconBar{margin-top: 96px;}
/* Remove margins from "page content" on small screens */
@media only screen and (max-width: 600px) {#main {margin-left: 0; margin-top: 60px;} #myNavbar{margin-top:116px}}
</style>
<body class="w3-indigo-extra-light">
  <!-- Header/Home -->
<header class="w3-container w3-white w3-top w3-text-indigo" id="home">
  <h1 class="w3-jumbo">
  <span>Bid$</span>
  <i class="fa fa-gavel"></i>
  </h1>
</header>
<!-- Icon Bar (Sidebar - hidden on small screens) -->
<nav class="w3-sidebar w3-bar-block w3-small w3-hide-small w3-center w3-text-indigo w3-indigo-extra-light" id="iconBar">
  <?php drawSidebar(); ?>
</nav>

<!-- Navbar on small screens (Hidden on medium and large screens) -->
<div class="w3-top w3-hide-large w3-hide-medium" id="myNavbar">
  <div class="w3-bar w3-indigo-light w3-center w3-small">
    <?php drawNavbar(); ?>
  </div>
</div>

<!-- Page Content -->
<div class="w3-padding-large" id="main">
  <!-- About Section -->
  <div class="w3-content w3-justify w3-text-grey w3-padding-64" id="about">
	<?php 
	      if(isset($_REQUEST['error'])) {
          $error = $_REQUEST['error'];

          echo '<div class="w3-content w3-card w3-justify w3-text-grey w3-white" id="error">';
          echo '<h1 class="w3-center w3-text-red">ERROR</h1>';
          echo '<p class="w3-center">';
          echo htmlentities($error);
          echo '</p>';
          echo '<button class="w3-indigo w3-hover-white w3-hover-text-indigo w3-center w3-button w3-block" onclick="hideErrorDIV()">';
          echo '<i class="fa fa-home w3-large"></i>';
          echo 'BACK TO HOME';
          echo '</button>';
          echo '</div>';
          echo '<div class="w3-row-padding hidden w3-animate-bottom" style="margin:0 -16px" id="page">';
        } else {
          $error = false;
          echo '<div class="w3-row-padding" style="margin:0 -16px" id="page">';
        }
?>

    <!-- Grid for pricing tables -->
    <!--<div class="w3-row-padding" style="margin:0 -16px">-->
        <ul class="w3-ul w3-padding">
        <?php drawUsersTHR(); ?>
          <!--<li>
            <h1>Bid #1</h1>
            
            <ul class="w3-ul w3-indigo w3-opacity w3-hover-opacity-off">
              <li>
              	<h1>your current offer:</h1>
              </li>
              <li class="w3-white"><h1 class="w3-large">2.50€</h1></li>
              <li class="w3-white w3-text-green hidden">you are the highest bidder</li>
              <li class="w3-white w3-text-red">bid exceeded</li>
            </ul>
            <br>
            
            <table class="w3-table w3-white w3-center">
              <tr>
                <th class="w3-dark-grey w3-xlarge w3-padding w3-center" style="width: 25%; height: 100%;"></th>
                <th class="w3-dark-grey w3-xlarge" style="border-style: solid; border-right:0px; border-top: 0px; border-bottom:0px; border-color:white" id="cur_text">your new offer:</th>
              </tr>
              <tr>
              	<td class="w3-dark-grey w3-center" style="border-style: solid; border-left:0px; border-top: 0px; border-bottom:0px; border-color:white"><i class="fa fa-star w3-jumbo w3-center"></i>
                </td>
                <td class="w3-white w3-xxlarge" id="cur_value">
                      <input type="text" id="thr" pattern="[0-9]{1,10}\.?[0-9]{1,2}" maxlength="13" style="width:75%">
                      <i class="fa fa-eur w3-xlarge"></i>
                </td>
              </tr>
              <tr>
              <td class="w3-dark-grey" style="border-style: solid; border-left:0px; border-top: 0px; border-bottom:0px; border-color:white"></td>
                <td class="w3-white">
                  <p class="w3-text-dark-grey" style="margin-top: -4px">enter EUR 1.00 or more</p>
		<p class="hidden" id="thr">1.00</p>
                </td>
              </tr>
              <tr>
              <td class="w3-dark-grey" style="border-style: solid; border-left:0px; border-top: 0px; border-bottom:0px; border-color:white"></td>
              	<th class="w3-indigo w3-hover-indigo-light w3-padding w3-center w3-large">PLACE BID</th>
              </tr>
          	</table>          
        	</li>
      	</ul>-->
    <!-- End Grid/Pricing tables -->
    </div>
  </div>

<script type="text/javascript">
var thr = document.getElementById('thr');
  
function hideErrorDIV(){
	var errdiv = document.getElementById('error');
	errdiv.style.display = 'none';

	var pagediv = document.getElementById('page');
	pagediv.style.display = 'block';
}

function isNumberAndGEMaxBid() {
  if(!isNaN(parseFloat(thr.value)) && isFinite(thr.value)){
  	thr.setCustomValidity('ciao');
  }else{
  	thr.setCustomValidity('Value must be a number');
  }
  
  if(parseFloat(thr.value) < parseFloat(maxbid)){
	thr.setCustomValidity("Your offer must be greater than or equal to current bid");
  }else{
	thr.setCustomValidity('value > maxnid');
  }
}

thr.onchange = isNumberAndGEMaxBid;

</script>
    
    <!-- Footer -->
  <footer class="w3-content w3-padding-64 w3-text-grey w3-xlarge">
    <!--<i class="fa fa-facebook-official w3-hover-opacity"></i>
    <i class="fa fa-instagram w3-hover-opacity"></i>
    <i class="fa fa-snapchat w3-hover-opacity"></i>
    <i class="fa fa-pinterest-p w3-hover-opacity"></i>
    <i class="fa fa-twitter w3-hover-opacity"></i>
    <i class="fa fa-linkedin w3-hover-opacity"></i>
    <p class="w3-medium">Powered by <a href="https://www.w3schools.com/w3css/default.asp" target="_blank" class="w3-hover-text-green">w3.css</a></p>-->
  <!-- End footer -->
  </footer>


<!-- END PAGE CONTENT -->
</div>

</body>
</html>

