<?php

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
    if (sizeof($_GET)) {
      // add a new argument
      header('Location: https://' . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI'] . '&cookies', TRUE, 301);
    } else {
      // this is the only argument
      header('Location: https://' . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI'] . '?cookies', TRUE, 301);
    }

    die();
  }

  if(count($_COOKIE) > 0){
    // ok
  } else {
    die('<h1 style="color:#fff;background-color:#f44336">you must enable cookies to view this site</h1>');
  }
}

//check if cookie check was already done
if (!isset($_COOKIE['test'])) {
  checkCookiesEnabled();
}

// inactivity period
$maxInactivityPeriod = 60 * 2;  //2 minutes

$database = 'local';
if ($database === 'local') {
  $host = 'localhost';
  $user = 'pma';
  $pass = 'phpMyAdmin';
  $db = 'test';
} else{
  //require 'cclix_db_credentials.php';
  //$host = $cclix_host; unset($cclix_host);
  //$user = $cclix_user; unset($cclix_user);
  //$pass = $cclix_pwd; unset($cclix_pwd);
  //$db = $cclix_db; unset($cclix_db);
}

?>