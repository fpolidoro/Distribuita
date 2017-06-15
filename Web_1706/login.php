<?php
      session_start();
    // remove all session variables because new login is asked
    session_unset();
    require 'functions.php';
    
    if(!isset($_POST['login'])) {    //login è il nome del button
        //redirectWithError('isset post register');
        $error = 'isset post login';
        header('Location: index.php?error='.urlencode($error));
        die();
    }else{
        $conn = dbConnect();
        if(!isset($_POST['usrname']) || !isset($_POST['pswLogin']) || $_POST['usrname'] === '' || $_POST['pswLogin'] === ''){
            //redirectWithError('Isset in login non va');
            $error = 'isset in login usrname et c non va';
            header('Location: index.php?error='.urlencode($error));
            die();
        }
        
        $email = $_POST['usrname'];
        $psw = $_POST['pswLogin'];

        //sanitize strings (trim removes spaces before and after the string)
        $email = $conn->real_escape_string(htmlentities(trim($_POST['usrname'])));
        $psw = $conn->real_escape_string($_POST['pswLogin']);

        if(!filter_var($email, FILTER_VALIDATE_EMAIL)){
            //email wrong format error
            //redirectWithError('email in wrong format');
            $error = 'email in wrong format';
            header('Location: index.php?error='.urlencode($error));
            die();
        }
               
        login($conn, $email, $psw);
    }
?>
