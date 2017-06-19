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
      }?>
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
          echo '<a class="w3-indigo w3-hover-white w3-hover-text-indigo w3-center w3-button w3-block" href="profile.php">';
          echo '<i class="fa fa-home w3-large"></i>';
          echo 'BACK TO HOME';
          echo '</a>';
          echo '</div>';
          die();
        }else{
          $error = false;
          echo '<div class="w3-row-padding" id="page">';
        }?>

        <ul class="w3-ul w3-padding">
        <?php drawUsersTHR(); ?>
	</ul>
    </div>
  </div>
   
    <!-- Footer -->
  <footer class="w3-content w3-padding-64 w3-text-grey w3-xlarge">
  </footer>


<!-- END PAGE CONTENT -->
</div>
</body>
</html>

