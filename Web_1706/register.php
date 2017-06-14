<?php
    session_start();
    // remove all session variables because new login is asked
    session_unset();

    require 'functions.php';
    
    if(!isset($_POST['register'])) {    //register è il nome del button
        //redirectWithError('isset post register');
        $error = 'isset post register';
        header('Location: index.php?error='.urlencode($error));
        die();
    }else{
        $conn = dbConnect();
        if(!isset($_POST['newusrname']) || !isset($_POST['newpsw']) || !isset($_POST['checkpsw']) || $_POST['email'] === '' || $_POST['newpsw'] === '' || $_POST['checkpsw'] === ''){
            //redirectWithError('Isset in register non va');
            $error = 'isset in register usrname et c non va';
            header('Location: index.php?error='.urlencode($error));
            die();
        }
        

        $email = $_POST['newusrname'];
        $psw1 = $_POST['newpsw'];
        $psw2 = $_POST['checkpsw'];

        //sanitize strings
        $email = $conn->real_escape_string(htmlentities(trim($_POST['newusrname'])));
        $psw1 = $conn->real_escape_string($_POST['newpsw']);
        $psw2 = $conn->real_escape_string($_POST['checkpsw']);

        if(!filter_var($email, FILTER_VALIDATE_EMAIL)){
            //email wrong format error
            //redirectWithError('email in wrong format');
            $error = 'email in wrong format';
            header('Location: index.php?error='.urlencode($error));
  die();
        }
        if($psw1 != $psw2){
            //passwords don't match error
            //redirectWithError('passwords do not match');
            $error = 'passwords do not match';
            header('Location: index.php?error='.urlencode($error));
  die();
        }
        $hashedpsw = hash('sha256', $psw1);
        
        signup($conn, $email, $hashedpsw);
    }
?>