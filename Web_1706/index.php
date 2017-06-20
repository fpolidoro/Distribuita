<?php
	require 'functions.php';
	isUserAuthenticated(false);

	$conn = dbConnect();
?>
<!DOCTYPE html>
<html>
<title>Bid$ - Home</title>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="stylesheet" href="libs/w3.css">
<link rel="stylesheet" href="libs/custom_w3_changes.css">
<link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Montserrat">
<link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css">
<body class="w3-indigo-extra-light">
  <!-- Header/Home -->
<header class="w3-container w3-white w3-top w3-text-indigo" id="home">
  <h1 class="w3-jumbo">
  <span>Bid$</span>
  <i class="fa fa-gavel"></i>
  </h1>
</header>

<script type="text/javascript">
window.onload = function() {
  document.getElementById("newpsw").onchange = validatePassword;
  document.getElementById("newpsw").onkeyup = doPasswordsMatch;
  document.getElementById("checkpsw").onkeyup = doPasswordsMatch;
  document.getElementById("usrname").onchange = validateEmail;
  document.getElementById("newusrname").onchange = validateEmail;
}  

function validatePassword(){
  var password = document.getElementById("newpsw"),
  pattern= /(?=.*[A-Za-z])(?=.*\d).{2,}/;

  if(password.value != ''){
    if(!pattern.test(password.value)) {
      password.setCustomValidity("Password must contain at least a number and a letter");
      document.getElementById("registerFailed").innerHTML = "Password does not meet the requested format";
    } else {
      confirm_password.setCustomValidity('');
      document.getElementById("registerFailed").innerHTML = "";
    }
  }
}

function doPasswordsMatch(){
  var password = document.getElementById("newpsw")
  , confirm_password = document.getElementById("checkpsw");

  if(password.value != confirm_password.value && (password.value != '' && confirm_password.value != '')) {
    confirm_password.setCustomValidity("Passwords don't match");
    document.getElementById("registerFailed").innerHTML = "passwords must match";
  } else {
    confirm_password.setCustomValidity('');
    document.getElementById("registerFailed").innerHTML = "";
  }
}

function validateEmail() {
  var pattern = /^[a-zA-Z0-9\-_]+(\.[a-zA-Z0-9\-_]+)*@[a-z0-9]+(\-[a-z0-9]+)*(\.[a-z0-9]+(\-[a-z0-9]+)*)*\.[a-z]{2,4}$/;

  if(document.getElementById('loginCard').style.display === 'block'){
    var email = document.getElementById('usrname');
    var msgbox = document.getElementById("loginFailed");
  }else {
    var email = document.getElementById('newusrname');
    var msgbox = document.getElementById("registerFailed");
  }
  
  if(email.value != ''){
    if (!pattern.test(email.value)) {
      email.setCustomValidity("Email does not appear to be in a valid format");
      msgbox.innerHTML = "Please insert a valid email address";
    } else {
      email.setCustomValidity('');
      msgbox.innerHTML = "";
    }
  }
}

function switchCards() {
    var login = document.getElementById('loginCard');
    var register = document.getElementById('registerCard');
    var logForm = document.getElementById('loginForm');
    var regForm = document.getElementById('registerForm');
    
    
    if (login.style.display === 'none') {
        login.style.display = 'block';
        register.style.display = 'none';
        document.getElementById('newusrname').setCustomValidity('');
        document.getElementById('newpsw').setCustomValidity('');
        document.getElementById('checkpsw').setCustomValidity('');
        document.getElementById('registerFailed').innerHTML = '';
        regForm.reset();
    } else {
        login.style.display = 'none';
        register.style.display = 'block';
        document.getElementById('usrname').setCustomValidity('');
        document.getElementById('loginFailed').innerHTML = '';
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
        document.getElementById('usrname').setCustomValidity('');
        document.getElementById('loginFailed').innerHTML = '';
        logForm.reset();
        login.style.display = 'none';
    }
    if(register.style.display = 'block'){
      document.getElementById('newusrname').setCustomValidity('');
      document.getElementById('newpsw').setCustomValidity('');
      document.getElementById('checkpsw').setCustomValidity('');
      document.getElementById('registerFailed').innerHTML = '';
      regForm.reset();
      register.style.display = 'none';
    }
}

function hideErrorDIV(){
	var errdiv = document.getElementById('error');
	errdiv.style.display = 'none';

	var pagediv = document.getElementById('page');
	pagediv.style.display = 'block';
}

</script>
<noscript>
<div id="jsbanner" class="w3-panel w3-display-container w3-indigo w3-center w3-animate-top w3-display-top">
  <h3>Javascript is disabled, the site may not work properly</h3>
</div>
</noscript>



<!-- Icon Bar (Sidebar - hidden on small screens) -->
<nav class="w3-sidebar w3-bar-block w3-small w3-hide-small w3-center w3-text-indigo w3-indigo-extra-light" id="iconBar">
  <?php
  if(!isset($_REQUEST['fatalError'])){
    drawSidebar();
  } ?>
</nav>

<!-- Navbar on small screens (Hidden on medium and large screens) -->
<div class="w3-top w3-hide-large w3-hide-medium" id="myNavbar">
  <div class="w3-bar w3-indigo-light w3-center w3-small">
    <?php
    if(!isset($_REQUEST['fatalError'])){
      drawNavbar();
      }
    ?>
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
		<p class="w3-animate-bottom w3-center w3-text-red" id="loginFailed"></p>
        
      	<div class="w3-section">
          <input class="w3-input w3-border w3-margin-bottom" type="email" placeholder="Email" maxlength="256" name="usrname" id="usrname" required>
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
          <input class="w3-input w3-border w3-margin-bottom" type="email" placeholder="Enter your email" name="newusrname" maxlength="256" id="newusrname" required>
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

<!-- Page Content -->
<div class="w3-padding-large" id="main">
  <div class="w3-content w3-justify w3-text-grey w3-padding-64">
	<?php 
	      if(isset($_REQUEST['error'])) {
          $error = $_REQUEST['error'];

          echo '<div class="w3-content w3-card w3-justify w3-text-grey w3-white w3-animate-top w3-padding" id="error" style="margin-top: 60px">';
          if($error == 'SessionTimeOut'){
            echo '<h1 class="w3-center w3-text-red">SESSION TIME OUT</h1>';
          }else{
            echo '<h1 class="w3-center w3-text-red">ERROR</h1>';
          }
          echo '<p class="w3-center">';
          if($error == 'SessionTimeOut'){
            echo 'Your session has expired.<br>You have been logged out due to inactivity.';
          }else{         
            echo htmlentities($error);
          }
          echo '</p>';
          echo '<button class="w3-indigo w3-hover-white w3-hover-text-indigo w3-center w3-button w3-block" onclick="hideErrorDIV()">';
          echo '<i class="fa fa-home w3-large"></i>';
          echo 'BACK TO HOME';
          echo '</button>';
          echo '</div>';
          echo '<div class="w3-row-padding hidden w3-animate-bottom" id="page">';
        }else if(isset($_REQUEST['fatalError'])){
          $fatalError = $_REQUEST['fatalError'];
          echo '<div class="w3-content w3-card w3-justify w3-text-grey w3-white w3-animate-top w3-padding" id="error" style="margin-top: 60px">';
          echo '<h1 class="w3-center w3-text-red">FATAL ERROR</h1>';
          echo '<p class="w3-center">' . htmlentities($fatalError) . '</p>';
          echo '<a class="w3-indigo w3-hover-white w3-hover-text-indigo w3-center w3-button w3-block" href="index.php">';
          echo '<i class="fa fa-home w3-large"></i>';
          echo 'BACK TO HOME';
          echo '</a>';
          echo '</div>';
          die();
        }else{
          $error = false;
          echo '<div class="w3-row-padding" id="page">';
        }
?>
  <ul class="w3-ul">
    <?php drawBids(); ?>
  </ul>
</div>
    
    <!-- Footer -->
  <footer class="w3-content w3-padding-64 w3-text-grey w3-xlarge">
  </footer>

<!-- END PAGE CONTENT -->
</div>
</div>

</body>
</html>

