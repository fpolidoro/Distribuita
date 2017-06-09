<?php
function redirectOnHTTPS(){
    if(empty($_SERVER['HTTPS']) || $_SERVER['HTTPS'] != on){
        //forzo https
        $redirect = 'https://.$_SERVER['HTTP_HOST']'.$_SERVER['REQUEST_URI'];
        header('HTTP/1.1 301 Moved Permanently');
        header('Location: '.$redirect);
        exit();
    }
}

?>