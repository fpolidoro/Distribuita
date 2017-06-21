<?php
ini_set('display_errors', 1);

// force HTTPS
if(empty($_SERVER['HTTPS']) || $_SERVER['HTTPS'] != 'on') {
  header('Location: https://' . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI'], TRUE, 301);
  die();
}


// check whether cookies are enabled on this browser, otherwise prevent access to the site
function checkCookiesEnabled() {
  // set a test cookie
  setcookie('test', 1);
  // redirect on test page if not already on it
  if(!isset($_GET['cookies'])){
    //set "cookies" for checking if test has been set, after redirection. if cookies is already set, don't redirect (avoids redirection loops)
    if (sizeof($_GET)) {
      // the url already contains arguments, so append &cookies
      header('Location: https://' . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI'] . '&cookies', TRUE, 301);
    } else {
      // the url has no parameters set, so append ?cookies
      header('Location: https://' . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI'] . '?cookies', TRUE, 301);
    }

    die();
  }

  if(count($_COOKIE) > 0){
    // ok
  } else {
    $page = '
            <html>
            <title>Bid$ - Cookies disabled</title>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <link rel="stylesheet" href="libs/w3.css">
            <link rel="stylesheet" href="libs/custom_w3_changes.css">
            <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Montserrat">
            <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css"><body class="w3-indigo-extra-light">
            <header class="w3-container w3-white w3-top w3-text-indigo" id="home">
            <h1 class="w3-jumbo"><span>Bid$</span><i class="fa fa-gavel"></i></h1></header>
            <div class="w3-padding-large" id="main">
            <div class="w3-content w3-justify w3-text-grey w3-padding-64">
            <div class="w3-content w3-card w3-justify w3-text-grey w3-white w3-animate-top w3-padding" id="error" style="margin-top: 60px">
            <h1 class="w3-center w3-text-red">COOKIES ARE DISABLED</h1>
            <p class="w3-center">You must enable cookies in order to navigate this site.</p>
            <a class="w3-indigo w3-hover-white w3-hover-text-indigo w3-center w3-button w3-block" href="index.php">
            <i class="fa fa-home w3-large"></i> HOME</a></div></div></div></body></html>';
    die($page);
  }
}

//check if cookie check was already done
if (!isset($_COOKIE['test'])) {
  checkCookiesEnabled();
}

// inactivity period
$maxInactivityPeriod = 60 * 2;  //2 minutes

$database = 'cclix11';
if ($database === 'local') {
  $host = 'localhost';
  $user = 'root';
  $pass = 'toor';
  $db = 's231594';
} else{
  $host = 'localhost';
  $user = 's231594';
  $pass = 'ilsindis';
  $db = 's231594';
}

?>