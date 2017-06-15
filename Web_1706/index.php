<?php
	require 'functions.php';
	isUserAuthenticated(false);
?>
<!DOCTYPE html>
<html>
<title>Bid$</title>
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

<!-- Login modal -->
  <div id="loginCard" class="w3-modal">
      <div class="w3-modal-content w3-card-4 w3-animate-left">
      <header class="w3-container w3-center">
      	<i class="fa fa-times w3-text-indigo w3-hover-text-red w3-xxlarge w3-display-topright w3-padding" onclick="closeAndResetCard()" title="Close Modal"></i>
        <p class="w3-medium w3-text-indigo">LOGIN to Bid$</p>
	  </header>
      <form class="w3-container" id="loginForm" method="POST" action="login.php">
		<p class="w3-animate-bottom w3-center w3-text-red" id="loginFailed"/>
        
      	<div class="w3-section">
          <input class="w3-input w3-border w3-margin-bottom" type="email" placeholder="Email" maxlength="128" name="usrname" required>
          <input class="w3-input w3-border" type="password" placeholder="Password" maxlength="128" name="pswLogin" required>
          <button class="w3-button w3-block w3-indigo w3-section w3-padding w3-hover-indigo-light" type="submit" name="login">Login</button>
        </div>
      </form>
      <div class="w3-container" style="padding-top:0px">
      <p class="w3-text-black w3-center">or</p>
      <button class="w3-button w3-block w3-section w3-indigo w3-padding w3-hover-indigo-light" onclick="switchCards()">Register</button>
      </div>
      </div>
  </div>

<!-- Register modal -->
    <div id="registerCard" class="w3-modal">
      <div class="w3-modal-content w3-card-4 w3-animate-right">
      <header class="w3-container w3-center">
      	<i class="fa fa-times w3-text-indigo w3-hover-text-red w3-xxlarge w3-display-topright w3-padding" onclick="closeAndResetCard()" title="Close Modal"></i>
        <p class="w3-medium w3-text-indigo">REGISTER to Bid$</p>
	  </header>
      <form class="w3-container" action="register.php" method="POST" id="registerForm">
		<p class="w3-animate-bottom w3-center w3-text-red" id="registerFailed"/>
        <div class="w3-section">
          <input class="w3-input w3-border w3-margin-bottom" type="email" placeholder="Enter your email" name="newusrname" maxlength="128" required>
          <input class="w3-input w3-border" type="password" placeholder="Enter a password*" name="newpsw" id="newpsw" maxlength="128" required pattern="(?=.*[A-Za-z])(?=.*\d).{2,}">
          <p class="w3-text-black w3-small">*Password must contain at least a number and a letter</p>
          <input class="w3-input w3-border" type="password" maxlength="128" placeholder="Re-enter password*" name="checkpsw" id="checkpsw" required>
          <button class="w3-button w3-block w3-indigo w3-section w3-padding w3-hover-indigo-light" type="submit" name="register">Register</button>
        </div>
      </form>

      <div class="w3-container w3-padding">
        <p class="w3-text-black w3-right">Already a member? <span onclick="switchCards()" class="w3-hover-text-red w3-text-indigo">Login</span></p>
      </div>
      </div>
  </div>

<script type="text/javascript">
var password = document.getElementById("newpsw")
  , confirm_password = document.getElementById("checkpsw");
  

function validatePassword(){
  if(password.value != confirm_password.value) {
    confirm_password.setCustomValidity("Passwords Don't Match");
    document.getElementById("registerFailed").innerHTML = "passwords must match";
  } else {
    confirm_password.setCustomValidity('');
    document.getElementById("registerFailed").innerHTML = "";
  }
}

password.onchange = validatePassword;
confirm_password.onkeyup = validatePassword;

function switchCards() {
    var login = document.getElementById('loginCard');
    var register = document.getElementById('registerCard');
    var logForm = document.getElementById('loginForm');
    var regForm = document.getElementById('registerForm');
    
    
    if (login.style.display === 'none') {
        login.style.display = 'block';
        register.style.display = 'none';
        regForm.reset();
    } else {
        login.style.display = 'none';
        register.style.display = 'block';
        logForm.reset();
    }
}

function showLoginCard(){
	document.getElementById('loginCard').style.display='block';
}

function closeAndResetCard(){
	var login = document.getElementById('loginCard');
    var register = document.getElementById('registerCard');
    var logForm = document.getElementById('loginForm');
    var regForm = document.getElementById('registerForm');
    
    if (login.style.display === 'block') {
        login.style.display = 'none';
        logForm.reset();
    }
    if(register.style.display = 'block'){
		register.style.display = 'none';
        regForm.reset();
    }
}

function hideErrorDIV(){
	var errdiv = document.getElementById('error');
	errdiv.style.display = 'none';

	var pagediv = document.getElementById('page');
	pagediv.style.display = 'block';
}
</script>


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
    	echo '<button class="w3-indigo w3-hover-white w3-hover-text-indigo w3-center w3-button" onclick="hideErrorDIV()">';
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
              <ul class="w3-ul">
        <li>
        <h1>Bid #1</h1>
        <table class="w3-ul w3-white w3-opacity w3-hover-opacity-off w3-center">
        <tr>
          <th rowspan="3" class="w3-dark-grey w3-xlarge w3-padding" style="width: 25%; height: 100%;"><i class="fa fa-asterisk w3-jumbo"></i></th>
          <th class="w3-dark-grey w3-xlarge" id="cur_text">starting from:</th>
        </tr>
        <tr>
          <td class="w3-white w3-xxxlarge" id="cur_value">1.00 <i class="fa fa-eur w3-xlarge"></i></td>
        </tr>
        <tr>
        <td class="w3-left w3-padding">offered by:<span class="w3-text-indigo w3-right" id="offered_by">p1@polito.it</span></td>
        </tr>
      </table>
      </li>
      </ul>
    <!-- End Grid/Pricing tables -->
    </div>
    
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

